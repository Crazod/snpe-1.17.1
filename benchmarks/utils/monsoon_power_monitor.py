#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import multiprocessing
import time
import monsoon


class PowerMonitorError(Exception):
    pass


def _monitor_power(device, is_collecting, output):
    """Monitoring process
       Args:
         device: A profiler.monsoon object to collect samples from.
         is_collecting: the event to synchronize on.
         output: opened file to write the samples.
    """
    output_fd = open(output, 'wt')
    with output_fd:
        samples = []
        start_time = None
        end_time = None
        try:
            device.StartDataCollection()
            is_collecting.set()
            # First sample also calibrate the computation.
            device.CollectData()
            start_time = time.time()
            while is_collecting.is_set():
                new_data = device.CollectData()
                assert new_data, 'Unable to collect data from device'
                samples += new_data
            end_time = time.time()
        finally:
            device.StopDataCollection()
        result = {
            'duration_s': end_time - start_time,
            'samples': samples
        }
        json.dump(result, output_fd)


class MonsoonPowerMonitor:
    initVoltage = 3.8
    initMaxCurrent = 8

    def __init__(self, output_log_file, logger):
        self._powermonitor_process = None
        self._powermonitor_output_file = output_log_file
        self._is_collecting = None
        self._monsoon = None
        self._logger = logger
        try:
            self._monsoon = monsoon.Monsoon(wait=False)
            # Nominal Li-ion voltage is 3.7V, but it puts out 4.2V at max capacity.
            # Use 4.0V to simulate a "~80%" charged battery. Google "li-ion voltage
            # curve". This is true only for a single cell. (Most smartphones, some
            # tablets.)
            assert not self.initVoltage < 3.7, ('Init voltage < 3.7')
            self._monsoon.SetVoltage(self.initVoltage)
            self._monsoon.SetUsbPassthrough(2)
            assert not self.initMaxCurrent > 8, ('Init max current > 8')
            self._monsoon.SetMaxCurrent(self.initMaxCurrent)
        except EnvironmentError as envError:
            self._logger.error(envError)
            self._monsoon = None

    def start_monitor(self):
        assert not self._powermonitor_process, (
            'Must call StopMonitoringPower().')
        assert self._monsoon, "monsoon is not found"
        self._logger.debug('Monsoon status')
        self._logger.debug(self._monsoon.GetStatus())
        self._logger.debug('%s StartMonitoringPower' % time.ctime())
        self._is_collecting = multiprocessing.Event()
        self._powermonitor_process = multiprocessing.Process(
            target=_monitor_power,
            args=(self._monsoon,
                  self._is_collecting,
                  self._powermonitor_output_file))
        # Ensure child is not left behind: parent kills daemonic children on exit.
        self._powermonitor_process.daemon = True
        self._powermonitor_process.start()
        if not self._is_collecting.wait(timeout=0.5):
            self._powermonitor_process.terminate()
            raise PowerMonitorError('Failed to start power data collection.')

    def stop_monitor(self):
        '''
        Stops monitoring power and return duration and sampled current and voltage.
        See _MonitorPower for output format
        '''
        assert self._powermonitor_process, (
            'StartMonitoringPower() not called.')
        self._logger.debug('%s StopMonitoringPower' % time.ctime())
        try:
            # Tell powermonitor to take an immediate sample and join.
            self._is_collecting.clear()
            self._powermonitor_process.join()
        finally:
            self._powermonitor_output_file = None
            self._powermonitor_process = None
            self._is_collecting = None
