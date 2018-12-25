import argparse
import enum
import fileinput
import logging
import sys


logger = logging.getLogger(__name__)


__all__ = [
    'Channel', 'MarkerType', 'Marker', 'EnrichableSpiAnalyzer'
]


class Channel(enum.Enum):
    SCK = 0
    MOSI = 1
    MISO = 2


class MarkerType(enum.Enum):
    ErrorDot = 0
    Square = 1
    ErrorSquare = 2
    UpArrow = 3
    DownArrow = 4
    X = 5
    ErrorX = 6
    Start = 7
    Stop = 8
    One = 9
    Zero = 10
    Dot = 11


class MessageType(enum.Enum):
    bubble = 0
    marker = 1
    tabular = 2


class Marker(object):
    def __init__(self, idx: int, direction: Channel, marker_type: MarkerType):
        self._idx = idx
        self._direction = direction
        self._marker_type = marker_type

    def __str__(self):
        return '{idx}\t{channel}\t{type}'.format(
            idx=self._idx,
            channel=self._direction.name.lower(),
            type=self._marker_type.name,
        )


class EnrichableSpiAnalyzer(object):
    def __init__(self, *args):
        self.args = args

        super(EnrichableSpiAnalyzer, self).__init__()

    def get_enabled_message_types(self):
        features = set()

        if hasattr(self, 'handle_marker'):
            features.add(MessageType.marker)
        if hasattr(self, 'handle_bubble'):
            features.add(MessageType.bubble)
        if hasattr(self, 'handle_tabular'):
            features.add(MessageType.tabular)

        return features

    def _call_handler(self, cmd, *args, **kwargs):
        handler_name = 'handle_{cmd}'.format(cmd=cmd)
        try:
            handler = getattr(self, handler_name)
        except AttributeError:
            logger.exception(
                "Although enabled, handler %s does not exist.",
                handler_name,
            )
            return

        try:
            return handler(*args, **kwargs)
        except Exception as e:
            logger.exception(
                "%s failed: %s; args: %s; kwargs: %s",
                handler_name,
                e,
                args,
                kwargs
            )
            return

    @classmethod
    def add_arguments(cls, parser):
        pass

    @classmethod
    def run(cls, sys_args):
        parser = argparse.ArgumentParser()
        parser.add_argument(
            '--loglevel',
            default='INFO',
            choices=[
                'CRITICAL',
                'ERROR',
                'WARNING',
                'INFO',
                'DEBUG',
            ]
        )
        cls.add_arguments(parser)
        args = parser.parse_args(sys_args)

        logging.basicConfig(level=getattr(logging, args.loglevel))
        logging.info(
            "Starting analyzer: {name}".format(
                name=cls.__name__
            )
        )
        instance = cls(args)
        instance.run_forever()

    def run_forever(self):
        enabled_features = self.get_enabled_message_types()

        for line in fileinput.input('-'):
            line = line.strip()

            logger.debug(">> %s", line)

            output_line = ""
            if line.startswith('bubble\t'):
                _, pkt, idx, start, end, f_type, flags, direction, value = (
                    line.split("\t")
                )

                results = self._call_handler(
                    "bubble",
                    int(pkt, 16) if pkt else None,
                    int(idx, 16),
                    int(start, 16),
                    int(end, 16),
                    int(f_type, 16),
                    int(flags, 16),
                    Channel[direction.upper()],
                    int(value, 16)
                )
                if results:
                    output_line = '\n'.join([str(r) for r in results]) + '\n'
            elif line.startswith('marker\t'):
                _, pkt, idx, count, start, end, f_type, flags, mosi, miso = (
                    line.split("\t")
                )

                results = self._call_handler(
                    "marker",
                    int(pkt, 16) if pkt else None,
                    int(idx, 16),
                    int(count, 16),
                    int(start, 16),
                    int(end, 16),
                    int(f_type, 16),
                    int(flags, 16),
                    int(mosi, 16),
                    int(miso, 16)
                )
                if results:
                    output_line = '\n'.join([str(r) for r in results]) + '\n'
            elif line.startswith('tabular\t'):
                _, pkt, idx, start, end, f_type, flags, mosi, miso = (
                    line.split("\t")
                )

                results = self._call_handler(
                    "tabular",
                    int(pkt, 16) if pkt else None,
                    int(idx, 16),
                    int(start, 16),
                    int(end, 16),
                    int(f_type, 16),
                    int(flags, 16),
                    int(mosi, 16),
                    int(miso, 16)
                )
                if results:
                    output_line = '\n'.join([str(r) for r in results]) + '\n'
            elif line.startswith('feature\t'):
                _, name = line.split("\t")

                result = False
                try:
                    message_type = MessageType[name]
                    if message_type in enabled_features:
                        result = True
                except KeyError:
                    pass

                output_line = 'yes' if result else 'no'

            logger.debug("<< %s", output_line)

            sys.stdout.write(output_line)
            sys.stdout.write('\n')
            sys.stdout.flush()
