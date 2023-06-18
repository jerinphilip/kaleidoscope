from PIL import Image as PILImage
from chalk import *
from colour import Color
from chalk import BoundingBox


import math
import random

from itertools import product
from argparse import ArgumentParser


random.seed(0)

# Data is a nested structure.
black = Color("#000000")
white = Color("#ffffff")
red = Color("#ff0000")
green = Color("#00ff00")
gold = Color("#ffff00")
blue = Color("#0000ff")

module = (
    "Module",
    white,
    red,
    [
        ("Target Information", gold, red, []),
        (
            "Global Symbols",
            gold,
            red,
            [
                ("[Global Variable]*", white, red, []),
                ("[Function Declaration]*", white, red, []),
                ("[Function Declaration]*", white, green, []),
            ],
        ),
        ("Other stuff", gold, red, []),
    ],
)

function = (
    "Function",
    white,
    green,
    [
        ("[Argument]*", gold, green, []),
        ("[Entry Basic Block]*", gold, green, []),
        ("[Basic Block]*", gold, blue, []),
    ],
)

basic_block = (
    "Basic Block",
    white,
    blue,
    [
        ("Label", white, green, []),
        ("[Phi Instruction]*", white, green, []),
        ("[Instruction]*", white, green, []),
    ],
)


LINE_WIDTH = 0.2


def bottom_up(data):
    label, background, line, children = data
    width, height = 20, 2
    if not children:
        radius = min(width, height) / 20
        container = (
            rectangle(width, height, radius)
            .line_color(line)
            .fill_color(background)
            .line_width(LINE_WIDTH)
        )
        bounding_rect = container.scale(0.9)
        render_label = (
            text(label, 1)
            .line_color(black)
            .fill_color(black)
            .with_envelope(bounding_rect)
        )
        return ((width, height), container + render_label)

    leaves = []
    widths, heights = [], []

    for child in children:
        (w, h), leaf = bottom_up(child)
        widths.append(w)
        heights.append(h)
        leaves.append(leaf)

    label_bounding_rectangle = rectangle(width, height, 1)
    render_label = (
        text(label, 1)
        .line_color(black)
        .fill_color(black)
        .with_envelope(label_bounding_rectangle)
    )

    h = sum(heights) + (1 * len(leaves) - 1)
    w = max(widths)

    leaves.insert(0, render_label)
    inside_render = vcat(leaves, 1)

    padding = 1
    padded_w, padded_h = w + padding, h + padding + 4
    radius = min(padded_w, padded_h) / 20
    container = (
        rectangle(padded_w, padded_h, radius)
        .line_color(line)
        .fill_color(background)
        .line_width(LINE_WIDTH)
    )

    render = container + inside_render.center_xy()
    return (padded_w, padded_h), render


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--path", type=str, required=True)
    args = parser.parse_args()

    dims, module_render = bottom_up(module)
    dims, function_render = bottom_up(function)
    dims, basic_block_render = bottom_up(basic_block)

    diagram = hcat([module_render, function_render, basic_block_render], 1).center_xy()

    diagram.render_svg(args.path, height=50)
