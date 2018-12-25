import datetime
import fileinput
import sys


def get_bubble_text(line):
    _, idx, start, end, f_type, flags, direction, value = (
        line.split('\t')
    )

    return "My sample message"


def get_markers(line):
    _, idx, sample_count, start, end, f_type, flags, mosi, miso = (
        line.split('\t')
    )

    markers = []

    if int(miso, 16) == 0xff:
        markers.append("0\tmiso\tStop")
    if int(miso, 16) == 0x00:
        markers.append("0\tmosi\tStart")

    return markers


def main(logfile, *args):
    for line in fileinput.input():
        line = line.strip()

        logfile.write(">> ")
        logfile.write(line)
        logfile.write('\n')
        logfile.flush()

        result = ""
        if line.startswith('bubble\t'):
            result = get_bubble_text(line)
        elif line.startswith('marker\t'):
            markers = get_markers(line)
            if markers:
                result = "\n".join(markers) + "\n"

        if not result:
            result = ""

        logfile.write("<< ")
        logfile.write(result)
        logfile.write("\n")
        sys.stdout.write(result)
        sys.stdout.write("\n")
        sys.stdout.flush()


if __name__ == '__main__':
    with open('/tmp/analyzer_log.txt', 'a') as logfile:
        logfile.write('Started {date}\n'.format(date=datetime.datetime.now()))
        logfile.flush()
        try:
            main(logfile, sys.argv[1:])
        except Exception as e:
            logfile.write(e)
            logfile.write(
                'Failed at {date}\n'.format(date=datetime.datetime.now())
            )
            logfile.flush()
