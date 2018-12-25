import argparse
import enum
import fileinput
import logging
import sys
from typing import List, Optional


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
    ENABLE_MARKER = True
    ENABLE_BUBBLE = True
    ENABLE_TABULAR = True

    def __init__(self, *args):
        self.args = args

        super(EnrichableSpiAnalyzer, self).__init__()

    def get_enabled_message_types(self):
        features = set()

        if self.ENABLE_MARKER:
            features.add(MessageType.marker)
        if self.ENABLE_BUBBLE:
            features.add(MessageType.bubble)
        if self.ENABLE_TABULAR:
            features.add(MessageType.tabular)

        return features

    def get_bubble_text(
        self,
        packet_id: Optional[int],
        frame_index: int,
        start_sample: int,
        end_sample: int,
        frame_type: int,
        flags: int,
        direction: Channel,
        value: int
    ) -> List[str]:
        return ""

    def _get_bubble_text(self, *args, **kwargs) -> str:
        try:
            return self.get_bubble_text(*args, **kwargs)
        except Exception as e:
            logger.exception(
                "get_bubble_text failed: %s; args: %s; kwargs: %s",
                e,
                args,
                kwargs
            )
            return ""

    def get_markers(
        self,
        packet_id: Optional[int],
        frame_index: int,
        sample_count: int,
        start_sample: int,
        end_sample: int,
        frame_type: int,
        flags: int,
        mosi_value: int,
        miso_value: int
    ) -> List[Marker]:
        return []

    def _get_markers(self, *args, **kwargs) -> List[str]:
        try:
            return self.get_markers(*args, **kwargs)
        except Exception as e:
            logger.exception(
                "get_markers failed: %s; args: %s; kwargs: %s",
                e,
                args,
                kwargs
            )
            return []

    def get_tabular(
        self,
        packet_id: Optional[int],
        frame_index: int,
        start_sample: int,
        end_sample: int,
        frame_type: int,
        flags: int,
        mosi_value: int,
        miso_value: int
    ) -> List[str]:
        miso_bubble = self.get_bubble_text(
            packet_id,
            frame_index,
            start_sample,
            end_sample,
            frame_type,
            flags,
            Channel.MISO,
            miso_value,
        )
        mosi_bubble = self.get_bubble_text(
            packet_id,
            frame_index,
            start_sample,
            end_sample,
            frame_type,
            flags,
            Channel.MOSI,
            mosi_value,
        )

        return [
            "MOSI: {mosi}; MISO: {miso}".format(
                miso=miso_bubble[0] if miso_bubble else hex(miso_value),
                mosi=mosi_bubble[0] if mosi_bubble else hex(mosi_value),
            )
        ]

    def _get_tabular(self, *args, **kwargs) -> str:
        try:
            return self.get_tabular(*args, **kwargs)
        except Exception as e:
            logger.exception(
                "get_tabular_text failed: %s; args: %s; kwargs: %s",
                e,
                args,
                kwargs
            )
            return ""

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

                results = self._get_bubble_text(
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

                results = self._get_markers(
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

                results = self._get_tabular(
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
