import csv
from collections import OrderedDict
from snpebm_constants import *


class CsvWriter:
    SPACE = ' '
    NOT_AVAILABLE = "N/A"
    SDK_VERSION_HEADER = "SNPE SDK version:"
    CONFIG_HEADER = "Configuration used:"
    DEVICE_INFO_HEADER = "Device Info:"
    UNITS = {MEASURE_TIMING: "us", MEASURE_MEM: "kB", MEASURE_POWER: "A"}

    def __init__(self, snpe_sdk_version, benchmarks, config, device_info, sleeptime):
        self._snpe_sdk_version = snpe_sdk_version
        self._tables = {}
        self._config = config
        self._device_info = device_info
        self._sleeptime = sleeptime
        for run_flavor_measure, bm in benchmarks:
            if bm._measurement.type not in self._tables:
                self._tables.update({bm._measurement.type: []})
            self._tables[bm._measurement.type].append(bm)

    def __write_metadata(self, writer):
        writer.writerow([self.SDK_VERSION_HEADER, self._snpe_sdk_version])
        writer.writerow([])
        writer.writerow([self.CONFIG_HEADER])
        rows = self._config.csvrows
        writer.writerows(rows)
        writer.writerow([])
        writer.writerow([self.DEVICE_INFO_HEADER])
        writer.writerows(self._device_info)
        writer.writerow([])
        return

    def write(self, csv_file_path, logger):
        csv_file = open(csv_file_path, 'wt')
        try:
            writer = csv.writer(csv_file)
            self.__write_metadata(writer)
            for measure_type, bms in self._tables.iteritems():
                header_row = [self.SPACE]
                header_row_2 = [self.SPACE]
                data_rows = OrderedDict()
                bmcount = 0
                for bm in bms:
                    if (self._sleeptime == 0):
                        header_row += ["{0}_{1}({2} runs)".format(bm._name, measure_type, self._config.iterations), self.SPACE, self.SPACE]
                    else:
                        header_row += ["{0}_{1}({2} runs, {3}s sleep)".format(bm._name, measure_type, self._config.iterations, self._sleeptime),
                                       self.SPACE, self.SPACE]
                    unit = self.UNITS[measure_type]
                    header_row_2 += ["avg ({0})".format(unit), "max ({0})".format(unit), "min ({0})".format(unit)]
                    avg_dict = bm._measurement.average
                    max_dict = bm._measurement.max
                    min_dict = bm._measurement.min
                    for channel, raw_data in avg_dict.iteritems():
                        if channel not in data_rows:
                            data_rows[channel] = [channel]
                            for i in range(0,bmcount):
                                #Add padding as needed.  One pad for "avg", one for "max", one for "min"
                                data_rows[channel] += [self.SPACE]
                                data_rows[channel] += [self.SPACE]
                                data_rows[channel] += [self.SPACE]
                        data_rows[channel] += [raw_data]

                    for channel, raw_data in max_dict.iteritems():
                        if channel not in data_rows:
                            logger.error("Error: invalid data")
                            return
                        data_rows[channel] += [raw_data]

                    for channel, raw_data in min_dict.iteritems():
                        if channel not in data_rows:
                            logger.error("Error: invalid data")
                            return
                        data_rows[channel] += [raw_data]

                    bmcount = bmcount + 1
                writer.writerow([""]+ [""] + header_row)
                writer.writerow([""]+ [""] + header_row_2)
                for channel in data_rows.keys():
                    writer.writerow([""] + [""] + data_rows[channel])
                writer.writerow([])

        finally:
            csv_file.close()

class AccuracyCsvWriter(CsvWriter):
    UNITS = {MEASURE_TIMING: "us", MEASURE_MEM: "kB", MEASURE_POWER: "A", MEASURE_ACCURACY: ""}

class CsvWriterFactory:
    def __init__(self):
        pass

    @staticmethod
    def make_csv_writer(benchmarks, config, device_info, sleeptime, logger):
        snpe_sdk_version = benchmarks[0][1].get_snpe_version(config, logger)
        if MEASURE_ACCURACY in config.measurements:
            return AccuracyCsvWriter(snpe_sdk_version, benchmarks, config, device_info, sleeptime)
        else:
            return CsvWriter(snpe_sdk_version, benchmarks, config, device_info, sleeptime)
