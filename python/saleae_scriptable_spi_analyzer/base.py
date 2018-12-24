import argparse
import enum
import fileinput
import logging
import sys
from typing import List


logger = logging.getLogger(__name__)


__all__ = [
    'Channel', 'MarkerType', 'Marker', 'ScriptableSpiAnalyzer'
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


class Marker(object):
    def __init__(self, idx: int, direction: Channel, marker_type: MarkerType):
        self._idx = idx
        self._direction = direction
        self._marker_type = marker_type

    def __str__(self):
        return '{idx}\t{channel}\t{type}'.format(
            idx=self._idx,
            channel=self._direction.name,
            type=self._marker_type.name,
        )


class ScriptableSpiAnalyzer(object):
    def __init__(self, *args):
        self.args = args

        super(ScriptableSpiAnalyzer, self).__init__()

    def get_bubble_text(
        self,
        frame_index: int,
        start_sample: int,
        end_sample: int,
        direction: Channel,
        value: int
    ) -> str:
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
        frame_index: int,
        sample_count: int,
        start_sample: int,
        end_sample: int,
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
        frame_index: int,
        start_sample: int,
        end_sample: int,
        mosi_value: int,
        miso_value: int
    ) -> str:
        return "MOSI: {mosi}; MISO: {miso}".format(
            mosi=self.get_bubble_text(
                frame_index,
                start_sample,
                end_sample,
                Channel.MISO,
                miso_value,
            ),
            miso=self.get_bubble_text(
                frame_index,
                start_sample,
                end_sample,
                Channel.MOSI,
                mosi_value,
            ),
        )

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
        for line in fileinput.input('-'):
            line = line.strip()

            logger.debug(">> %s", line)

            output_line = ""
            if line.startswith('bubble\t'):
                _, idx, start, end, direction, value = line.split("\t")

                result = self._get_bubble_text(
                    int(idx, 16),
                    int(start, 16),
                    int(end, 16),
                    Channel[direction.upper()],
                    int(value, 16)
                )
                if result:
                    output_line = str(result)
            elif line.startswith('marker\t'):
                _, idx, sample_count, start, end, mosi, miso = line.split("\t")

                results = self._get_markers(
                    int(idx, 16),
                    int(sample_count, 16),
                    int(start, 16),
                    int(end, 16),
                    int(mosi, 16),
                    int(miso, 16)
                )
                if results:
                    output_line = '\n'.join([str(r) for r in results]) + '\n'
            elif line.startswith('tabular\t'):
                _, idx, start, end, mosi, miso = line.split("\t")

                result = self._get_tabular(
                    int(idx, 16),
                    int(start, 16),
                    int(end, 16),
                    int(mosi, 16),
                    int(miso, 16)
                )
                if result:
                    output_line = str(result)

            logger.debug("<< %s", output_line)

            sys.stdout.write(output_line)
            sys.stdout.write('\n')
            sys.stdout.flush()
