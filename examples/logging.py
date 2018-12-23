import datetime
import fileinput
import sys


def handle_line(line):
    yield "bubble\tMy sample message"


def main(logfile, *args):
    for line in fileinput.input():
        line = line.strip()

        logfile.write(">> ")
        logfile.write(line)
        logfile.write('\n')
        logfile.flush()

        for result in handle_line(line):
            logfile.write("<< ")
            logfile.write(result)
            logfile.write("\n")

            sys.stdout.write(result)
            sys.stdout.write("\n")
            sys.stdout.flush()

        logfile.write("<< ")
        logfile.write("\n")
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
            logfile.write('Ended {date}\n'.format(date=datetime.datetime.now()))
            logfile.flush()
