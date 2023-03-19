#!/usr/bin/env python3
#
# This file is formatted with Python Black
#
# Run with pytest

from pathlib import Path

import configparser
import os
import pytest
import re


def quirksdir():
    return Path(os.getenv("MESON_SOURCE_ROOT") or ".") / "quirks"


def pytest_generate_tests(metafunc):
    # for any function that takes a "quirksfile" argument return the path to
    # a quirks file
    if "quirksfile" in metafunc.fixturenames:
        metafunc.parametrize("quirksfile", [f for f in quirksdir().glob("*.quirks")])


def test_matches_are_valid(quirksfile):
    quirks = configparser.ConfigParser(strict=True)
    # Don't convert to lowercase
    quirks.optionxform = lambda option: option  # type: ignore
    quirks.read(quirksfile)

    for name, section in filter(lambda n: n != "DEFAULT", quirks.items()):
        bus = section.get("MatchBus")
        if bus is not None:
            assert bus in ("ps2", "usb", "bluetooth", "i2c", "spi")

        vid = section.get("MatchVendor")
        if vid is not None:
            assert re.match(
                "0x[0-9A-F]{4}", vid
            ), f"{quirksfile}: {name}: {vid} must be uppercase hex (0xAB12)"

        pid = section.get("MatchProduct")
        if pid is not None:
            assert re.match(
                "0x[0-9A-F]{4}", pid
            ), f"{quirksfile}: {name}: {pid} must be uppercase hex (0xAB12)"


def main():
    args = [__file__]
    try:
        import xdist  # noqa

        ncores = os.environ.get("FDO_CI_CONCURRENT", "auto")
        args += ["-n", ncores]
    except ImportError:
        pass

    return pytest.main(args)


if __name__ == "__main__":
    raise SystemExit(main())
