#!/usr/bin/env python3
"""Generate the KiCad schematic source and routed PCB for the temperature monitor.

Run with KiCad's bundled Python so pcbnew is available:
    D:\\KiCad\\10.0\\bin\\python.exe generate_kicad.py

The schematic is emitted in the legacy text format because it is compact and
deterministic.  Opening and saving it once in KiCad 10 converts it to the
current .kicad_sch format.  The PCB is written directly with KiCad's pcbnew API.
"""

from __future__ import annotations

import heapq
import math
import re
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

KICAD_ROOT = Path(r"D:\KiCad\10.0")
sys.path.insert(0, str(KICAD_ROOT / "bin"))
import pcbnew  # type: ignore  # noqa: E402

PROJECT = Path(__file__).resolve().parents[1]
NAME = "TemperatureInspector_STM32F103C8T6_Hardware"
SCHEMATIC_ROOT_UUID = "e3993a41-ab6f-4c1c-822f-48c5ef1db4a0"
SYMBOL_DIR = KICAD_ROOT / "share" / "kicad" / "symbols"
FOOTPRINT_DIR = KICAD_ROOT / "share" / "kicad" / "footprints"


@dataclass
class Part:
    ref: str
    lib: str
    value: str
    footprint: str
    sch: tuple[int, int]
    pcb: tuple[float, float] | None
    rotation: float
    nets: dict[str, str]


def part(ref, lib, value, footprint, sch, pcb, rotation, nets):
    return Part(ref, lib, value, footprint, sch, pcb, rotation, {str(k): v for k, v in nets.items()})


P = []
add = P.append

# USB-C power input and regulated 3.3 V rail.
add(part("J1", "Connector:USB_C_Receptacle_PowerOnly_6P", "USB-C 5V POWER ONLY",
         "Connector_USB:USB_C_Receptacle_GCT_USB4125-xx-x_6P_TopMnt_Horizontal",
         (1900, 1700), (10, 18), 90,
         {"A5": "USB_CC1", "A9": "USB_5V", "A12": "GND",
          "B5": "USB_CC2", "B9": "USB_5V", "B12": "GND", "SH": "GND"}))
add(part("R1", "Device:R", "5.1k", "Resistor_SMD:R_0603_1608Metric", (3300, 1350), (18, 11), 0,
         {1: "USB_CC1", 2: "GND"}))
add(part("R2", "Device:R", "5.1k", "Resistor_SMD:R_0603_1608Metric", (3900, 1350), (22, 11), 0,
         {1: "USB_CC2", 2: "GND"}))
add(part("F1", "Device:Polyfuse", "1A PTC", "Fuse:Fuse_1206_3216Metric", (3300, 2100), (18, 25), 0,
         {1: "USB_5V", 2: "+5V"}))
add(part("D1", "Device:D_TVS", "SMBJ5.0A", "Diode_SMD:D_SMA", (2700, 2700), (20, 18), 90,
         {1: "USB_5V", 2: "GND"}))
add(part("U2", "Regulator_Linear:AP2112K-3.3", "AP2112K-3.3", "Package_TO_SOT_SMD:SOT-23-5",
         (4700, 2100), (24, 22), 0, {1: "+5V", 2: "GND", 3: "+5V", 5: "+3V3"}))
add(part("C1", "Device:C", "1u", "Capacitor_SMD:C_0603_1608Metric", (4100, 2750), (22, 28), 0,
         {1: "+5V", 2: "GND"}))
add(part("C2", "Device:C", "4.7u", "Capacitor_SMD:C_0805_2012Metric", (5400, 2750), (28, 25), 0,
         {1: "+3V3", 2: "GND"}))
add(part("#FLG0101", "power:PWR_FLAG", "PWR_FLAG", "", (3500, 2550), None, 0, {1: "+5V"}))
add(part("#FLG0102", "power:PWR_FLAG", "PWR_FLAG", "", (2250, 3050), None, 0, {1: "GND"}))

# MCU, reset/boot and local decoupling.
MCU_NETS = {
    1: "+3V3", 2: "BOARD_LED", 7: "NRST", 8: "GND", 9: "+3V3",
    10: "DS18B20_DQ", 11: "PT100_ADC", 12: "UART2_TX_ESP_RX", 13: "UART2_RX_ESP_TX",
    18: "BUZZER", 19: "ALARM_LED", 23: "GND", 24: "+3V3", 25: "SD_CS",
    26: "SD_SCK", 27: "SD_MISO", 28: "SD_MOSI", 30: "DEBUG_TX", 31: "DEBUG_RX",
    34: "SWDIO", 35: "GND", 36: "+3V3", 37: "SWCLK", 39: "KEY1", 40: "KEY2",
    41: "KEY3", 42: "I2C_SCL", 43: "I2C_SDA", 44: "BOOT0", 47: "GND", 48: "+3V3",
    49: "GND",
}
add(part("U1", "MCU_ST_STM32F1:STM32F103CBTx", "STM32F103CBT6",
         "Package_QFP:LQFP-48_7x7mm_P0.5mm", (8050, 5750), (55, 35), 0, MCU_NETS))
add(part("R3", "Device:R", "10k", "Resistor_SMD:R_0603_1608Metric", (5100, 4200), (43, 25), 0,
         {1: "+3V3", 2: "NRST"}))
add(part("C7", "Device:C", "100n", "Capacitor_SMD:C_0603_1608Metric", (5650, 4750), (46, 25), 0,
         {1: "NRST", 2: "GND"}))
add(part("SW4", "Switch:SW_Push", "RESET", "Button_Switch_THT:SW_PUSH_6mm", (5000, 5000), (49, 18), 0,
         {1: "NRST", 2: "GND"}))
add(part("R4", "Device:R", "10k", "Resistor_SMD:R_0603_1608Metric", (6050, 4200), (60, 26), 0,
         {1: "BOOT0", 2: "GND"}))
add(part("JP1", "Connector_Generic:Conn_01x02", "BOOT0", "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
         (6100, 5000), (60, 10), 0, {1: "+3V3", 2: "BOOT0"}))
for ref, value, sch, pcb in [
    ("C3", "100n", (6800, 9050), (46, 32)), ("C4", "100n", (7450, 9050), (64, 28)),
    ("C5", "100n", (8100, 9050), (57, 44)), ("C6", "1u VDDA", (8750, 9050), (48, 39)),
]:
    add(part(ref, "Device:C", value, "Capacitor_SMD:C_0603_1608Metric", sch, pcb, 0,
             {1: "+3V3", 2: "GND"}))

# Sensor and peripheral connectors.  Pin order is printed on the PCB; modules connect by wire.
add(part("J2", "Connector_Generic:Conn_01x03", "DS18B20: 3V3,DQ,GND",
         "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical", (12700, 1750), (10, 40), 0,
         {1: "+3V3", 2: "DS18B20_DQ", 3: "GND"}))
add(part("R5", "Device:R", "4.7k", "Resistor_SMD:R_0603_1608Metric", (11450, 1750), (13, 48), 0,
         {1: "+3V3", 2: "DS18B20_DQ"}))
add(part("J3", "Connector_Generic:Conn_01x04", "PT100: 5V,3V3,VOUT,GND",
         "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical", (12700, 3000), (18, 40), 0,
         {1: "+5V", 2: "+3V3", 3: "PT100_RAW", 4: "GND"}))
add(part("R6", "Device:R", "1k", "Resistor_SMD:R_0603_1608Metric", (11200, 3000), (21, 54), 0,
         {1: "PT100_RAW", 2: "PT100_ADC"}))
add(part("C8", "Device:C", "100n", "Capacitor_SMD:C_0603_1608Metric", (11600, 3450), (25, 56), 0,
         {1: "PT100_ADC", 2: "GND"}))
add(part("J4", "Connector_Generic:Conn_01x04", "OLED: GND,3V3,SCL,SDA",
         "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical", (12700, 4250), (27, 40), 0,
         {1: "GND", 2: "+3V3", 3: "I2C_SCL", 4: "I2C_SDA"}))
add(part("R7", "Device:R", "4.7k", "Resistor_SMD:R_0603_1608Metric", (11150, 4100), (29, 54), 0,
         {1: "+3V3", 2: "I2C_SCL"}))
add(part("R8", "Device:R", "4.7k", "Resistor_SMD:R_0603_1608Metric", (11650, 4100), (33, 54), 0,
         {1: "+3V3", 2: "I2C_SDA"}))
add(part("J5", "Connector_Generic:Conn_01x06", "SD: 3V3,CS,SCK,MISO,MOSI,GND",
         "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical", (12700, 5600), (65, 40), 0,
         {1: "+3V3", 2: "SD_CS", 3: "SD_SCK", 4: "SD_MISO", 5: "SD_MOSI", 6: "GND"}))
add(part("R9", "Device:R", "10k", "Resistor_SMD:R_0603_1608Metric", (11350, 5350), (69, 44), 0,
         {1: "+3V3", 2: "SD_CS"}))
add(part("J6", "Connector_Generic:Conn_01x04", "UART1: 3V3,TX,RX,GND",
         "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical", (12700, 6950), (92, 43), 0,
         {1: "+3V3", 2: "DEBUG_TX", 3: "DEBUG_RX", 4: "GND"}))
add(part("J7", "Connector_Generic:Conn_01x04", "SWD: 3V3,SWDIO,SWCLK,GND",
         "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical", (12700, 8250), (100, 43), 0,
         {1: "+3V3", 2: "SWDIO", 3: "SWCLK", 4: "GND"}))

# ESP-01S module and required boot pull-ups.
add(part("U3", "RF_Module:ESP-01", "ESP-01S", "RF_Module:ESP-01", (10900, 9300), (88, 28), 0,
         {1: "GND", 2: "ESP_GPIO2", 3: "ESP_GPIO0", 4: "UART2_TX_ESP_RX",
          5: "UART2_RX_ESP_TX", 6: "ESP_EN", 7: "ESP_RST", 8: "+3V3"}))
for ref, net, schx, pc in [
    ("R10", "ESP_GPIO2", 10100, (78, 38)), ("R11", "ESP_GPIO0", 10600, (82, 38)),
    ("R12", "ESP_EN", 11100, (86, 38)), ("R13", "ESP_RST", 11600, (90, 38)),
]:
    add(part(ref, "Device:R", "10k", "Resistor_SMD:R_0603_1608Metric", (schx, 8450), pc, 0,
             {1: "+3V3", 2: net}))
add(part("C9", "Device:C", "10u", "Capacitor_SMD:C_0805_2012Metric", (12100, 9300), (82, 34), 0,
         {1: "+3V3", 2: "GND"}))
add(part("C10", "Device:C", "100n", "Capacitor_SMD:C_0603_1608Metric", (12600, 9300), (86, 34), 0,
         {1: "+3V3", 2: "GND"}))

# User controls, buzzer driver and status LEDs.
for ref, net, sx, px in [("SW1", "KEY1", 2200, 75), ("SW2", "KEY2", 3500, 62), ("SW3", "KEY3", 4800, 49)]:
    add(part(ref, "Switch:SW_Push", ref.replace("SW", "KEY"), "Button_Switch_THT:SW_PUSH_6mm",
             (sx, 5250), (px, 68), 0, {1: net, 2: "GND"}))
add(part("BZ1", "Device:Buzzer", "5V ACTIVE BUZZER", "Buzzer_Beeper:Buzzer_12x9.5RM7.6", (2750, 7600), (68, 18), 0,
         {1: "+5V", 2: "BUZZER_COL"}))
add(part("Q1", "Transistor_BJT:Q_NPN_BEC", "MMBT3904", "Package_TO_SOT_SMD:SOT-23", (4300, 7800), (70, 32), 0,
         {1: "BUZZER_BASE", 2: "GND", 3: "BUZZER_COL"}))
add(part("R14", "Device:R", "1k", "Resistor_SMD:R_0603_1608Metric", (3550, 7800), (66, 32), 0,
         {1: "BUZZER", 2: "BUZZER_BASE"}))
add(part("R15", "Device:R", "100k", "Resistor_SMD:R_0603_1608Metric", (4000, 8350), (67, 35), 0,
         {1: "BUZZER_BASE", 2: "GND"}))
add(part("D2", "Device:D", "1N4148", "Diode_SMD:D_SOD-123", (3350, 7200), (78, 27), 90,
         {1: "+5V", 2: "BUZZER_COL"}))
for ref, value, signal, anode, rref, schy, pcbd, pcbr in [
    ("D3", "ALARM RED", "ALARM_LED", "ALARM_LED_A", "R16", 8900, (73, 50), (69, 50)),
    ("D4", "HEARTBEAT GREEN", "BOARD_LED", "BOARD_LED_A", "R17", 9700, (82, 50), (78, 50)),
    ("D5", "POWER GREEN", "+3V3", "POWER_LED_A", "R18", 10500, (91, 55), (87, 55)),
]:
    add(part(rref, "Device:R", "1k" if rref != "R17" else "2.2k", "Resistor_SMD:R_0603_1608Metric",
             (2800, schy), pcbr, 0, {1: signal, 2: anode}))
    add(part(ref, "Device:LED", value, "LED_SMD:LED_0603_1608Metric", (3600, schy), pcbd, 0,
             {1: "GND", 2: anode}))

for n, xy in enumerate([(30, 10), (80, 10), (10, 80), (100, 80)], 1):
    add(part(f"H{n}", "Mechanical:MountingHole", "M3", "MountingHole:MountingHole_3.2mm_M3",
             (14500, 9000 + n * 350), xy, 0, {}))


def schematic_symbol_uuids():
    uid = 0x61000000
    result = {}
    for component in P:
        if component.ref.startswith("H"):
            continue
        uid += 1
        result[component.ref] = f"00000000-0000-0000-0000-{uid:012x}"
    return result


SYMBOL_UUID_BY_REF = schematic_symbol_uuids()


def tokenize(text: str):
    return re.findall(r'"(?:\\.|[^"\\])*"|\(|\)|[^\s()]+', text)


def parse_tokens(tokens):
    stack = []
    root = []
    cur = root
    for tok in tokens:
        if tok == "(":
            node = []
            cur.append(node)
            stack.append(cur)
            cur = node
        elif tok == ")":
            cur = stack.pop()
        else:
            cur.append(tok[1:-1].replace(r'\"', '"') if tok.startswith('"') else tok)
    return root[0]


_symbol_files = {}


def load_symbol_file(nickname: str):
    if nickname not in _symbol_files:
        _symbol_files[nickname] = parse_tokens(tokenize((SYMBOL_DIR / f"{nickname}.kicad_sym").read_text(encoding="utf-8")))
    return _symbol_files[nickname]


def child(node, name):
    return next((x for x in node[1:] if isinstance(x, list) and x and x[0] == name), None)


def symbol_node(root, name):
    return next(x for x in root[1:] if isinstance(x, list) and len(x) > 1 and x[0] == "symbol" and x[1] == name)


def symbol_pin_data(lib_id: str):
    nickname, name = lib_id.split(":", 1)
    root = load_symbol_file(nickname)

    def resolved_nodes(sym):
        ext = child(sym, "extends")
        nodes = []
        if ext:
            nodes.extend(resolved_nodes(symbol_node(root, ext[1])))
        nodes.append(sym)
        return nodes

    pins = {}
    for sym in resolved_nodes(symbol_node(root, name)):
        all_units = [x for x in sym[1:] if isinstance(x, list) and len(x) > 1 and x[0] == "symbol"]
        units = [x for x in all_units if x[1].endswith("_1_1")]
        if not units:
            units = all_units
        for unit in units:
            for p in unit[1:]:
                if not (isinstance(p, list) and p and p[0] == "pin"):
                    continue
                at = child(p, "at")
                num = child(p, "number")
                name_node = child(p, "name")
                if at and num:
                    pins[str(num[1])] = (float(at[1]), float(at[2]), float(at[3]),
                                         str(name_node[1]) if name_node else "")
    if not pins and name != "MountingHole":
        raise RuntimeError(f"No pins found for {lib_id}")
    return pins


def symbol_pins(lib_id: str):
    return {number: data[:3] for number, data in symbol_pin_data(lib_id).items()}


def symbol_pin_names(lib_id: str):
    return {number: data[3] for number, data in symbol_pin_data(lib_id).items()}


def write_legacy_schematic():
    lines = [
        "EESchema Schematic File Version 4", f"LIBS:{NAME}-cache", "EELAYER 29 0", "EELAYER END",
        "$Descr A3 16535 11693", "Sheet 1 1", 'Title "STM32F103CBT6 Temperature Inspector"',
        'Date "2026-07-15"', 'Rev "1.0"', 'Comp "jiayu-here / Codex"',
        'Comment1 "USB-C 5V power only; all external modules use 3.3V logic"',
        'Comment2 "Firmware pinout matches TemperatureInspector_STM32F103C8T6.ioc"', "$EndDescr",
    ]
    notes = [
        (1500, 700, "POWER INPUT AND 3.3V REGULATION"), (6500, 700, "MCU / RESET / DECOUPLING"),
        (10800, 700, "SENSOR AND PERIPHERAL CONNECTORS"), (1500, 4300, "KEYS"),
        (1500, 6650, "BUZZER AND STATUS LED DRIVERS"), (9900, 7900, "ESP-01S (OPTIONAL)"),
        (8700, 3400, "PT100 VOUT MUST REMAIN BELOW 3.3V"),
    ]
    for x, y, text in notes:
        lines += [f"Text Notes {x} {y} 0    70   ~ 16", text]

    uid = 0x61000000
    labeled_points = set()
    for c in P:
        if c.ref.startswith("H"):
            continue
        uid += 1
        x, y = c.sch
        if c.ref == "U1":
            ref_x, ref_y = x - 1000, y - 1850
            value_x, value_y = x - 1000, y - 2050
        elif c.ref == "U3":
            ref_x, ref_y = x - 700, y - 650
            value_x, value_y = x - 700, y - 850
        else:
            ref_x, ref_y = x + 180, y - 180
            value_x, value_y = x + 220, y + 180
        lines += [
            "$Comp", f"L {c.lib} {c.ref}", f"U 1 1 {uid:08X}", f"P {x} {y}",
            f'F 0 "{c.ref}" H {ref_x} {ref_y} 40  0000 C CNN',
            f'F 1 "{c.value}" H {value_x} {value_y} 40  0000 C CNN',
            f'F 2 "{c.footprint}" H {x} {y} 50  0001 C CNN', f'F 3 "" H {x} {y} 50  0001 C CNN',
            f"\t1    {x} {y}", "\t1    0    0    -1", "$EndComp",
        ]
        pins = symbol_pins(c.lib)
        for number, (px_mm, py_mm, angle) in pins.items():
            px = x + round(px_mm * 39.37007874)
            py = y - round(py_mm * 39.37007874)
            if number in c.nets:
                point_key = (px, py, c.nets[number])
                if point_key in labeled_points:
                    continue
                labeled_points.add(point_key)
                if c.ref == "U1" or c.ref.startswith("J"):
                    stub = 500
                elif c.ref in {"R16", "R17", "R18"}:
                    stub = 100
                else:
                    stub = 150
                pin_angle = int(round(angle)) % 360
                if pin_angle == 0:
                    lx, ly = px - stub, py
                elif pin_angle == 90:
                    lx, ly = px, py + stub
                elif pin_angle == 180:
                    lx, ly = px + stub, py
                elif pin_angle == 270:
                    lx, ly = px, py - stub
                else:
                    raise RuntimeError(f"Unsupported pin angle {angle} for {c.lib} pin {number}")
                label_orientation = 1 if c.ref == "U1" and pin_angle in {90, 270} else 0
                lines += ["Wire Wire Line", f"\t{px} {py} {lx} {ly}",
                          f"Text Label {lx} {ly} {label_orientation}    30   ~ 0", c.nets[number]]
            else:
                lines.append(f"NoConn ~ {px} {py}")
    lines.append("$EndSCHEMATC")
    (PROJECT / f"{NAME}.sch").write_text("\n".join(lines) + "\n", encoding="utf-8")


def mm(v):
    return pcbnew.FromMM(v)


def vec(x, y):
    return pcbnew.VECTOR2I(mm(x), mm(y))


def footprint_load(identifier: str):
    lib, name = identifier.split(":", 1)
    fp = pcbnew.FootprintLoad(str(FOOTPRINT_DIR / f"{lib}.pretty"), name)
    if fp is None:
        raise RuntimeError(f"Cannot load footprint {identifier}")
    return fp


def add_track(board, net, layer, a, b, width):
    if a == b:
        return
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(vec(*a))
    t.SetEnd(vec(*b))
    t.SetLayer(layer)
    t.SetWidth(mm(width))
    t.SetNet(net)
    board.Add(t)


def add_via(board, net, xy, diameter=0.4, drill=0.2):
    pos = vec(*xy)
    for fp in board.GetFootprints():
        for pad in fp.Pads():
            pp = pad.GetPosition()
            if (pad.GetNetCode() == net.GetNetCode() and pad.GetDrillSize().x > 0
                    and abs(pp.x - pos.x) <= mm(0.2) and abs(pp.y - pos.y) <= mm(0.2)):
                return pad
    for item in board.GetTracks():
        if isinstance(item, pcbnew.PCB_VIA) and item.GetPosition() == pos and item.GetNetCode() == net.GetNetCode():
            return item
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(pos)
    v.SetWidth(mm(diameter))
    v.SetDrill(mm(drill))
    v.SetNet(net)
    board.Add(v)
    return v


class Router:
    def __init__(self, board, net_by_name, pad_terms, bounds=(5.7, 104.3, 5.7, 84.3), step=0.25):
        self.board = board
        self.nets = net_by_name
        self.pad_terms = pad_terms
        self.x0, self.x1, self.y0, self.y1 = bounds
        self.step = step
        self.nx = int(round((self.x1 - self.x0) / step)) + 1
        self.ny = int(round((self.y1 - self.y0) / step)) + 1
        self.block = [defaultdict(set), defaultdict(set)]
        self.tree = defaultdict(set)
        self._mark_pads()

    def grid(self, x, y):
        return (int(round((x - self.x0) / self.step)), int(round((y - self.y0) / self.step)))

    def world(self, ix, iy):
        return (self.x0 + ix * self.step, self.y0 + iy * self.step)

    def inside(self, ix, iy):
        return 0 <= ix < self.nx and 0 <= iy < self.ny

    def _mark_rect(self, layer, rect, owner, extra):
        xmin, ymin, xmax, ymax = rect
        ia, ja = self.grid(xmin - extra, ymin - extra)
        ib, jb = self.grid(xmax + extra, ymax + extra)
        for ix in range(max(0, ia), min(self.nx - 1, ib) + 1):
            x = self.world(ix, 0)[0]
            for iy in range(max(0, ja), min(self.ny - 1, jb) + 1):
                y = self.world(0, iy)[1]
                dx = max(xmin - x, 0, x - xmax)
                dy = max(ymin - y, 0, y - ymax)
                if math.hypot(dx, dy) <= extra + 1e-9:
                    self.block[layer][(ix, iy)].add(owner)

    def _mark_pads(self):
        clearance_to_track = 0.25
        for fp in self.board.GetFootprints():
            for pad in fp.Pads():
                owner = (pad.GetNetname() or "__NO_NET__").removeprefix("/")
                box = pad.GetBoundingBox()
                rect = (pcbnew.ToMM(box.GetX()), pcbnew.ToMM(box.GetY()),
                        pcbnew.ToMM(box.GetRight()), pcbnew.ToMM(box.GetBottom()))
                for li, layer in enumerate((pcbnew.F_Cu, pcbnew.B_Cu)):
                    if pad.IsOnLayer(layer):
                        self._mark_rect(li, rect, owner, clearance_to_track)

    def allowed(self, layer, cell, net, exceptions=frozenset()):
        owners = self.block[layer].get(cell, set())
        return cell in exceptions or not (owners - {net})

    def via_ok(self, cell, net):
        ix, iy = cell
        radius = 0.45
        r = math.ceil(radius / self.step)
        for dx in range(-r, r + 1):
            for dy in range(-r, r + 1):
                if math.hypot(dx * self.step, dy * self.step) > radius:
                    continue
                c = (ix + dx, iy + dy)
                if not self.inside(*c):
                    return False
                if (self.block[0].get(c, set()) | self.block[1].get(c, set())) - {net}:
                    return False
        return True

    def astar(self, starts, goals, net, max_nodes=500000, allow_vias=True):
        goals = set(goals)
        exceptions = {c for c, _ in starts} | {c for c, _ in goals}
        gx = [c[0] for c, _ in goals]
        gy = [c[1] for c, _ in goals]
        minx, maxx, miny, maxy = min(gx), max(gx), min(gy), max(gy)

        def h(cell):
            x, y = cell
            return max(minx - x, 0, x - maxx) + max(miny - y, 0, y - maxy)

        heap = []
        prev = {}
        best = {}
        for cell, layer in starts:
            state = (cell[0], cell[1], layer)
            best[state] = 0
            heapq.heappush(heap, (h(cell), 0, state))
            prev[state] = None
        nodes = 0
        while heap and nodes < max_nodes:
            _f, cost, state = heapq.heappop(heap)
            if cost != best.get(state):
                continue
            nodes += 1
            ix, iy, layer = state
            if ((ix, iy), layer) in goals:
                out = []
                while state is not None:
                    out.append(state)
                    state = prev[state]
                return list(reversed(out))
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                cell = (ix + dx, iy + dy)
                if not self.inside(*cell) or not self.allowed(layer, cell, net, exceptions):
                    continue
                ns = (cell[0], cell[1], layer)
                nc = cost + 1
                if nc < best.get(ns, 10**18):
                    best[ns] = nc
                    prev[ns] = state
                    heapq.heappush(heap, (nc + h(cell), nc, ns))
            if allow_vias and self.via_ok((ix, iy), net):
                ns = (ix, iy, 1 - layer)
                nc = cost + 45
                if nc < best.get(ns, 10**18):
                    best[ns] = nc
                    prev[ns] = state
                    heapq.heappush(heap, (nc + h((ix, iy)), nc, ns))
        return None

    def mark_track(self, net, layer, a, b, width):
        ax, ay = self.grid(*a)
        bx, by = self.grid(*b)
        steps = max(abs(bx - ax), abs(by - ay), 1)
        inflate = (width / 2 + 0.15 + 0.09 + 0.05) / self.step
        rr = math.ceil(inflate)
        for n in range(steps + 1):
            ix = round(ax + (bx - ax) * n / steps)
            iy = round(ay + (by - ay) * n / steps)
            for dx in range(-rr, rr + 1):
                for dy in range(-rr, rr + 1):
                    if math.hypot(dx, dy) <= inflate + 1e-9 and self.inside(ix + dx, iy + dy):
                        self.block[layer][(ix + dx, iy + dy)].add(net)

    def commit_path(self, net_name, path, width=0.18):
        net = self.nets[net_name]
        points = [(self.world(ix, iy), layer) for ix, iy, layer in path]
        # Compress collinear runs while preserving layer changes.
        runs = []
        start = 0
        for i in range(1, len(points)):
            if points[i][1] != points[i - 1][1]:
                if i - 1 > start:
                    runs.append((points[start][0], points[i - 1][0], points[i - 1][1]))
                xy = points[i][0]
                add_via(self.board, net, xy)
                self._mark_via(net_name, self.grid(*xy))
                start = i
                continue
            if i >= 2 and points[i - 1][1] == points[i - 2][1]:
                a, b, c = points[i - 2][0], points[i - 1][0], points[i][0]
                if (round(b[0] - a[0], 6), round(b[1] - a[1], 6)) != (round(c[0] - b[0], 6), round(c[1] - b[1], 6)):
                    runs.append((points[start][0], points[i - 1][0], points[i - 1][1]))
                    start = i - 1
        if len(points) - 1 > start:
            runs.append((points[start][0], points[-1][0], points[-1][1]))
        for a, b, layer in runs:
            layer_id = pcbnew.F_Cu if layer == 0 else pcbnew.B_Cu
            add_track(self.board, net, layer_id, a, b, width)
            self.mark_track(net_name, layer, a, b, width)
        for ix, iy, layer in path:
            self.tree[net_name].add(((ix, iy), layer))

    def _mark_via(self, net, cell):
        r = math.ceil(0.45 / self.step)
        for layer in (0, 1):
            for dx in range(-r, r + 1):
                for dy in range(-r, r + 1):
                    if math.hypot(dx * self.step, dy * self.step) <= 0.45 and self.inside(cell[0] + dx, cell[1] + dy):
                        self.block[layer][(cell[0] + dx, cell[1] + dy)].add(net)

    def connect_ground_smd(self):
        net_name = "GND"
        net = self.nets[net_name]
        for pad in self.pad_terms[net_name]:
            if pad.IsOnLayer(pcbnew.B_Cu):
                continue
            pos = pad.GetPosition()
            ref = pad.GetParentFootprint().GetReference()
            if ref == "U1" and pad.GetNumber() in {"8", "23", "35", "47"}:
                # Deterministic radial fan-out for the four LQFP48 ground pads.
                px, py = pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y)
                offsets = {"8": (-1.85, 0), "23": (0, 1.85), "35": (1.85, 0), "47": (0, -1.85)}
                dx, dy = offsets[pad.GetNumber()]
                target = (px + dx, py + dy)
                add_track(self.board, net, pcbnew.F_Cu, (px, py), target, 0.4)
                self.mark_track(net_name, 0, (px, py), target, 0.4)
                add_via(self.board, net, target)
                self._mark_via(net_name, self.grid(*target))
                continue
            start_cell = self.grid(pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y))
            goals = []
            for radius in range(4, 33):
                for dx in range(-radius, radius + 1):
                    for dy in (-radius, radius):
                        c = (start_cell[0] + dx, start_cell[1] + dy)
                        if self.inside(*c) and self.via_ok(c, net_name):
                            goals.append((c, 0))
                for dy in range(-radius + 1, radius):
                    for dx in (-radius, radius):
                        c = (start_cell[0] + dx, start_cell[1] + dy)
                        if self.inside(*c) and self.via_ok(c, net_name):
                            goals.append((c, 0))
            path = self.astar([(start_cell, 0)], goals, net_name, max_nodes=40000, allow_vias=False) if goals else None
            add_plane_via = bool(path)
            if not path:
                # Dense connectors such as USB-C cannot fit a via beside every
                # signal pad.  Join those pads to the closest plated GND pad.
                plated_goals = []
                for other in self.pad_terms[net_name]:
                    if other is pad or not other.IsOnLayer(pcbnew.B_Cu):
                        continue
                    op = other.GetPosition()
                    cell = self.grid(pcbnew.ToMM(op.x), pcbnew.ToMM(op.y))
                    plated_goals.extend([(cell, 0), (cell, 1)])
                path = self.astar([(start_cell, 0)], plated_goals, net_name, max_nodes=120000, allow_vias=True)
                add_plane_via = False
            if not path:
                raise RuntimeError(f"Unable to fan out GND pad {pad.GetParentFootprint().GetReference()}:{pad.GetNumber()}")
            self.commit_path(net_name, path, width=0.4)
            if add_plane_via:
                end = path[-1]
                xy = self.world(end[0], end[1])
                add_via(self.board, net, xy)
                self._mark_via(net_name, (end[0], end[1]))

    def route_net(self, net_name, width=0.18):
        pads = self.pad_terms[net_name]
        if len(pads) < 2 and not self.tree[net_name]:
            return
        pad_states = []
        for pad in pads:
            pos = pad.GetPosition()
            cell = self.grid(pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y))
            layers = []
            if pad.IsOnLayer(pcbnew.F_Cu):
                layers.append(0)
            if pad.IsOnLayer(pcbnew.B_Cu):
                layers.append(1)
            pad_states.append((pad, cell, layers))

        if not self.tree[net_name]:
            first = pad_states.pop(0)
            for layer in first[2]:
                self.tree[net_name].add((first[1], layer))
        while pad_states:
            goals = self.tree[net_name]
            pad_states.sort(key=lambda item: min(abs(item[1][0] - g[0][0]) + abs(item[1][1] - g[0][1]) for g in goals))
            pad, cell, layers = pad_states.pop(0)
            starts = [(cell, layer) for layer in layers]
            path = self.astar(starts, goals, net_name)
            if not path:
                raise RuntimeError(f"Unable to route {net_name} to {pad.GetParentFootprint().GetReference()}:{pad.GetNumber()}")
            self.commit_path(net_name, path, width)


def add_zone(board, net, layer):
    z = pcbnew.ZONE(board)
    z.SetLayer(layer)
    z.SetNet(net)
    z.SetLocalClearance(mm(0.2))
    z.SetMinThickness(mm(0.2))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    outline = z.Outline()
    outline.NewOutline()
    for p in (vec(5.8, 5.8), vec(104.2, 5.8), vec(104.2, 84.2), vec(5.8, 84.2)):
        outline.Append(p)
    board.Add(z)


def add_text(board, text, xy, size=1.2, layer=pcbnew.F_SilkS, justify=None):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(text)
    t.SetPosition(vec(*xy))
    t.SetLayer(layer)
    t.SetTextSize(vec(size, size))
    t.SetTextThickness(mm(max(0.15, size * 0.13)))
    if layer == pcbnew.B_SilkS:
        t.SetMirrored(True)
    if justify is not None:
        t.SetHorizJustify(justify)
    board.Add(t)


def write_board():
    board = pcbnew.BOARD()
    board.SetCopperLayerCount(4)
    settings = board.GetDesignSettings()
    settings.m_MinClearance = mm(0.15)
    settings.m_TrackMinWidth = mm(0.18)
    settings.m_ViasMinSize = mm(0.4)
    settings.m_ViasMinAnnularWidth = mm(0.1)
    settings.m_MinThroughDrill = mm(0.2)
    settings.m_HoleToHoleMin = mm(0.15)
    settings.m_HoleClearance = mm(0.15)
    settings.m_CopperEdgeClearance = mm(0.3)
    default = board.GetAllNetClasses()["Default"]
    default.SetClearance(mm(0.15))
    default.SetTrackWidth(mm(0.18))
    default.SetViaDiameter(mm(0.4))
    default.SetViaDrill(mm(0.2))
    default.SetDiffPairWidth(mm(0.18))
    default.SetDiffPairGap(mm(0.18))
    default.SetDiffPairViaGap(mm(0.18))

    unconnected_by_pad = {}
    for c in P:
        if not c.footprint or c.pcb is None or c.ref.startswith("H"):
            continue
        for number, pin_name in symbol_pin_names(c.lib).items():
            if number not in c.nets:
                unconnected_by_pad[(c.ref, number)] = f"unconnected-({c.ref}-{pin_name}-Pad{number})"

    net_names = sorted({n for c in P for n in c.nets.values()} | set(unconnected_by_pad.values()))
    net_by_name = {}
    for name in net_names:
        board_name = name if name.startswith("unconnected-(") else f"/{name}"
        ni = pcbnew.NETINFO_ITEM(board, board_name)
        board.Add(ni)
        net_by_name[name] = ni

    pad_terms = defaultdict(list)
    for c in P:
        if not c.footprint or c.pcb is None:
            continue
        fp = footprint_load(c.footprint)
        fp.SetFPIDAsString(c.footprint)
        fp.SetReference(c.ref)
        fp.SetValue(c.value)
        fp.Reference().SetVisible(False)
        fp.Value().SetVisible(False)
        fp.SetPosition(vec(*c.pcb))
        fp.SetOrientationDegrees(180 if c.ref == "C3" else c.rotation)
        if c.ref.startswith("H"):
            fp.SetBoardOnly(True)
        else:
            fp.SetPath(pcbnew.KIID_PATH(
                f"/{SCHEMATIC_ROOT_UUID}/{SYMBOL_UUID_BY_REF[c.ref]}"))
        board.Add(fp)
        terminal_keys = set()
        for pad in fp.Pads():
            number = str(pad.GetNumber())
            if number in c.nets:
                name = c.nets[number]
                pad.SetNet(net_by_name[name])
                key = (c.ref, number)
                if key not in terminal_keys:
                    pad_terms[name].append(pad)
                    terminal_keys.add(key)
            elif (c.ref, number) in unconnected_by_pad:
                pad.SetNet(net_by_name[unconnected_by_pad[(c.ref, number)]])

    # 100 x 80 mm board outline.
    for a, b in [((5, 5), (105, 5)), ((105, 5), (105, 85)), ((105, 85), (5, 85)), ((5, 85), (5, 5))]:
        s = pcbnew.PCB_SHAPE(board)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetLayer(pcbnew.Edge_Cuts)
        s.SetStart(vec(*a))
        s.SetEnd(vec(*b))
        s.SetWidth(mm(0.1))
        board.Add(s)

    router = Router(board, net_by_name, pad_terms)
    # The 6 mm tactile switches have two physical pads for each electrical
    # contact.  Join each same-number pair so connectivity is explicit to DRC.
    for ref in ("SW1", "SW2", "SW3", "SW4"):
        fp = board.FindFootprintByReference(ref)
        groups = defaultdict(list)
        for pad in fp.Pads():
            if pad.GetNetname():
                groups[str(pad.GetNumber())].append(pad)
        for pads in groups.values():
            if len(pads) != 2:
                continue
            points = []
            for pad in pads:
                pos = pad.GetPosition()
                points.append((pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y)))
            add_track(board, pads[0].GetNet(), pcbnew.F_Cu, points[0], points[1], 0.5)
            router.mark_track(pads[0].GetNetname().removeprefix("/"), 0, points[0], points[1], 0.5)
    # USB-C has overlapping A/B VBUS pads.  Escape left of the two alignment
    # holes, join on B.Cu, then return to F.Cu for the fuse and TVS branch.
    usb_net = net_by_name["USB_5V"]
    usb_pads = [p for p in pad_terms["USB_5V"] if p.GetParentFootprint().GetReference() == "J1"]
    fuse_pad = next(p for p in pad_terms["USB_5V"] if p.GetParentFootprint().GetReference() == "F1")
    usb_points = sorted({(round(pcbnew.ToMM(p.GetPosition().x), 3), round(pcbnew.ToMM(p.GetPosition().y), 3)) for p in usb_pads})
    fp = fuse_pad.GetPosition()
    fuse_point = (pcbnew.ToMM(fp.x), pcbnew.ToMM(fp.y))
    escape_x = 5.9
    trunk_x = 13.0
    for p in usb_points:
        escape = (escape_x, p[1])
        add_track(board, usb_net, pcbnew.F_Cu, p, escape, 0.4)
        router.mark_track("USB_5V", 0, p, escape, 0.4)
        add_via(board, usb_net, escape)
        router._mark_via("USB_5V", router.grid(*escape))
        add_track(board, usb_net, pcbnew.B_Cu, escape, (escape_x, 18.0), 0.6)
        router.mark_track("USB_5V", 1, escape, (escape_x, 18.0), 0.6)
    for layer, a, b in [
        (pcbnew.B_Cu, (escape_x, 18.0), (trunk_x, 18.0)),
        (pcbnew.F_Cu, (trunk_x, 18.0), (trunk_x, fuse_point[1])),
        (pcbnew.F_Cu, (trunk_x, fuse_point[1]), fuse_point),
    ]:
        if layer == pcbnew.F_Cu and a == (trunk_x, 18.0):
            add_via(board, usb_net, a)
            router._mark_via("USB_5V", router.grid(*a))
        add_track(board, usb_net, layer, a, b, 0.6)
        router.mark_track("USB_5V", 0 if layer == pcbnew.F_Cu else 1, a, b, 0.6)
    tvs_pad = next(p for p in pad_terms["USB_5V"] if p.GetParentFootprint().GetReference() == "D1")
    dp = tvs_pad.GetPosition()
    tvs_point = (pcbnew.ToMM(dp.x), pcbnew.ToMM(dp.y))
    for a, b in [(fuse_point, (18.0, 22.0)), ((18.0, 22.0), (20.0, 22.0)),
                 ((20.0, 22.0), tvs_point)]:
        add_track(board, usb_net, pcbnew.F_Cu, a, b, 0.6)
        router.mark_track("USB_5V", 0, a, b, 0.6)

    def route_cc(net_name, resistor_ref, via_in, b_path):
        net = net_by_name[net_name]
        source = next(p for p in pad_terms[net_name] if p.GetParentFootprint().GetReference() == "J1")
        target = next(p for p in pad_terms[net_name] if p.GetParentFootprint().GetReference() == resistor_ref)
        sp, tp = source.GetPosition(), target.GetPosition()
        source_xy = (pcbnew.ToMM(sp.x), pcbnew.ToMM(sp.y))
        target_xy = (pcbnew.ToMM(tp.x), pcbnew.ToMM(tp.y))
        via_out = b_path[-1]
        add_track(board, net, pcbnew.F_Cu, source_xy, via_in, 0.18)
        router.mark_track(net_name, 0, source_xy, via_in, 0.18)
        add_via(board, net, via_in)
        router._mark_via(net_name, router.grid(*via_in))
        points = [via_in] + b_path
        for a, b in zip(points, points[1:]):
            add_track(board, net, pcbnew.B_Cu, a, b, 0.18)
            router.mark_track(net_name, 1, a, b, 0.18)
        add_via(board, net, via_out)
        router._mark_via(net_name, router.grid(*via_out))
        add_track(board, net, pcbnew.F_Cu, via_out, target_xy, 0.18)
        router.mark_track(net_name, 0, via_out, target_xy, 0.18)

    r1p = next(p for p in pad_terms["USB_CC1"] if p.GetParentFootprint().GetReference() == "R1").GetPosition()
    r2p = next(p for p in pad_terms["USB_CC2"] if p.GetParentFootprint().GetReference() == "R2").GetPosition()
    route_cc("USB_CC1", "R1", (9.0, 19.1), [(14.0, 19.1), (14.0, pcbnew.ToMM(r1p.y))])
    route_cc("USB_CC2", "R2", (9.0, 16.6), [(9.0, 8.0), (20.0, 8.0),
                                                     (20.0, pcbnew.ToMM(r2p.y))])

    # Fan out every used LQFP48 signal before general routing.  Alternating
    # distances keep 0.5/0.25 mm vias clear at the 0.5 mm package pitch.
    for net_name, pads in list(pad_terms.items()):
        for pad in list(pads):
            if pad.GetParentFootprint().GetReference() != "U1":
                continue
            pp = pad.GetPosition()
            px, py = pcbnew.ToMM(pp.x), pcbnew.ToMM(pp.y)
            dx, dy = px - 55.0, py - 35.0
            distance = 1.5 if int(pad.GetNumber()) % 2 else 2.65
            if abs(dx) > abs(dy):
                target = (px + math.copysign(distance, dx), py)
            else:
                target = (px, py + math.copysign(distance, dy))
            target = router.world(*router.grid(*target))
            if net_name == "+3V3" and pad.GetNumber() == "9":
                target = (target[0], py + 0.02)
            width = 0.3 if net_name == "GND" else 0.18
            add_track(board, net_by_name[net_name], pcbnew.F_Cu, (px, py), target, width)
            router.mark_track(net_name, 0, (px, py), target, width)
            cell = router.grid(*target)
            if net_name == "BOARD_LED":
                router.tree[net_name].add((cell, 0))
            else:
                add_via(board, net_by_name[net_name], target)
                router._mark_via(net_name, cell)
                router.tree[net_name].add((cell, 1))
            pads.remove(pad)

    ground_seeds = sorted(router.tree["GND"])
    # +3V3 has several MCU supply pads, each with its own fan-out via.  Join
    # those seed vias into one real tree before adding the remaining loads.
    power_seeds = sorted(router.tree["+3V3"])
    router.tree["+3V3"] = {power_seeds[0]}
    for seed in power_seeds[1:]:
        path = router.astar([seed], router.tree["+3V3"], "+3V3", allow_vias=False)
        if not path:
            raise RuntimeError("Unable to join +3V3 MCU fan-out vias")
        router.commit_path("+3V3", path, width=0.18)
    router.route_net("BOARD_LED", width=0.18)
    router.route_net("BOARD_LED_A", width=0.18)

    # C3 sits left of the MCU.  Take its 3V3 pad directly to the nearest MCU
    # supply fan-out on B.Cu so it does not cross the I2C and key routes.
    c3_power = next(p for p in pad_terms["+3V3"]
                    if p.GetParentFootprint().GetReference() == "C3")
    cp = c3_power.GetPosition()
    c3_xy = (pcbnew.ToMM(cp.x), pcbnew.ToMM(cp.y))
    c3_via = router.world(*router.grid(c3_xy[0], c3_xy[1] + 1.2))
    nearest_seed = min(power_seeds,
                       key=lambda s: abs(s[0][0] - router.grid(*c3_via)[0])
                       + abs(s[0][1] - router.grid(*c3_via)[1]))
    supply_xy = router.world(*nearest_seed[0])
    add_track(board, net_by_name["+3V3"], pcbnew.F_Cu, c3_xy, c3_via, 0.18)
    router.mark_track("+3V3", 0, c3_xy, c3_via, 0.18)
    add_via(board, net_by_name["+3V3"], c3_via)
    router._mark_via("+3V3", router.grid(*c3_via))
    c3_b_points = [c3_via, (c3_via[0], 34.2), (supply_xy[0], 34.2), supply_xy]
    for a, b in zip(c3_b_points, c3_b_points[1:]):
        add_track(board, net_by_name["+3V3"], pcbnew.B_Cu, a, b, 0.18)
        router.mark_track("+3V3", 1, a, b, 0.18)
    pad_terms["+3V3"] = [p for p in pad_terms["+3V3"]
                           if p.GetParentFootprint().GetReference() != "C3"]
    # The four SPI2 signals reverse order between the MCU side and the SD
    # header.  Use staggered B.Cu corridors and return to F.Cu at the header so
    # the bus is deterministic and crossing-free.
    sd_corridors = {
        "SD_MOSI": 58.7,
        "SD_MISO": 59.2,
        "SD_SCK": 59.7,
        "SD_CS": 60.2,
    }
    sd_header_pads = {}
    for net_name in ("SD_MOSI", "SD_MISO", "SD_SCK", "SD_CS"):
        header_pad = next(p for p in pad_terms[net_name]
                          if p.GetParentFootprint().GetReference() == "J5")
        sd_header_pads[net_name] = header_pad
        hp = header_pad.GetPosition()
        header_xy = (pcbnew.ToMM(hp.x), pcbnew.ToMM(hp.y))
        source_cell = next(iter(router.tree[net_name]))[0]
        source_xy = router.world(*source_cell)
        corridor = sd_corridors[net_name]
        end_via = router.world(*router.grid(corridor, header_xy[1]))
        for a, b in [(source_xy, (corridor, source_xy[1])),
                     ((corridor, source_xy[1]), end_via)]:
            add_track(board, net_by_name[net_name], pcbnew.B_Cu, a, b, 0.18)
            router.mark_track(net_name, 1, a, b, 0.18)
        add_via(board, net_by_name[net_name], end_via)
        router._mark_via(net_name, router.grid(*end_via))
        add_track(board, net_by_name[net_name], pcbnew.F_Cu, end_via, header_xy, 0.18)
        router.mark_track(net_name, 0, end_via, header_xy, 0.18)

    # The SD CS pull-up is approached from its signal-side pad to avoid the
    # adjacent +3V3 pad on R9.
    cs_header = sd_header_pads["SD_CS"].GetPosition()
    cs_resistor = next(p for p in pad_terms["SD_CS"]
                       if p.GetParentFootprint().GetReference() == "R9").GetPosition()
    ch = (pcbnew.ToMM(cs_header.x), pcbnew.ToMM(cs_header.y))
    cr = (pcbnew.ToMM(cs_resistor.x), pcbnew.ToMM(cs_resistor.y))
    for a, b in [(ch, (66.5, ch[1])), ((66.5, ch[1]), (66.5, 41.0)),
                 ((66.5, 41.0), (72.0, 41.0)), ((72.0, 41.0), (72.0, cr[1])),
                 ((72.0, cr[1]), cr)]:
        add_track(board, net_by_name["SD_CS"], pcbnew.F_Cu, a, b, 0.18)
        router.mark_track("SD_CS", 0, a, b, 0.18)

    # Keep the PT100 RC filter branch compact and deterministic.
    pt_net = net_by_name["PT100_ADC"]
    pt_r = next(p for p in pad_terms["PT100_ADC"] if p.GetParentFootprint().GetReference() == "R6")
    pt_c = next(p for p in pad_terms["PT100_ADC"] if p.GetParentFootprint().GetReference() == "C8")
    pr, pc = pt_r.GetPosition(), pt_c.GetPosition()
    pt_a = (pcbnew.ToMM(pr.x), pcbnew.ToMM(pr.y))
    pt_b = (pcbnew.ToMM(pc.x), pcbnew.ToMM(pc.y))
    add_track(board, pt_net, pcbnew.F_Cu, pt_a, pt_b, 0.18)
    router.mark_track("PT100_ADC", 0, pt_a, pt_b, 0.18)
    pad_terms["PT100_ADC"] = [p for p in pad_terms["PT100_ADC"]
                                if p.GetParentFootprint().GetReference() != "C8"]

    # Pull-up branches are added after the MCU-to-header I2C trunks so they do
    # not trap the grid router at the OLED header pads.
    scl_j = next(p for p in pad_terms["I2C_SCL"] if p.GetParentFootprint().GetReference() == "J4")
    scl_r = next(p for p in pad_terms["I2C_SCL"] if p.GetParentFootprint().GetReference() == "R7")
    pad_terms["I2C_SCL"] = [p for p in pad_terms["I2C_SCL"]
                             if p.GetParentFootprint().GetReference() != "J4"]
    sda_j = next(p for p in pad_terms["I2C_SDA"] if p.GetParentFootprint().GetReference() == "J4")
    sda_r = next(p for p in pad_terms["I2C_SDA"] if p.GetParentFootprint().GetReference() == "R8")
    pad_terms["I2C_SDA"] = [p for p in pad_terms["I2C_SDA"]
                             if p.GetParentFootprint().GetReference() != "J4"]

    def connect_i2c_pullups():
        sj, sr = scl_j.GetPosition(), scl_r.GetPosition()
        scl_points = [(pcbnew.ToMM(sj.x), pcbnew.ToMM(sj.y)),
                      (pcbnew.ToMM(sr.x), pcbnew.ToMM(sj.y)),
                      (pcbnew.ToMM(sr.x), pcbnew.ToMM(sr.y))]
        for a, b in zip(scl_points, scl_points[1:]):
            add_track(board, net_by_name["I2C_SCL"], pcbnew.F_Cu, a, b, 0.18)
            router.mark_track("I2C_SCL", 0, a, b, 0.18)

        dj, dr = sda_j.GetPosition(), sda_r.GetPosition()
        sda_j_xy = (pcbnew.ToMM(dj.x), pcbnew.ToMM(dj.y))
        sda_r_xy = (pcbnew.ToMM(dr.x), pcbnew.ToMM(dr.y))
        sda_via = router.world(*router.grid(sda_r_xy[0] + 1.2, sda_r_xy[1]))
        add_track(board, net_by_name["I2C_SDA"], pcbnew.F_Cu, sda_r_xy, sda_via, 0.18)
        router.mark_track("I2C_SDA", 0, sda_r_xy, sda_via, 0.18)
        add_via(board, net_by_name["I2C_SDA"], sda_via)
        router._mark_via("I2C_SDA", router.grid(*sda_via))
        sda_points = [sda_via, (sda_via[0], 48.95), (sda_j_xy[0], 48.95), sda_j_xy]
        for a, b in zip(sda_points, sda_points[1:]):
            add_track(board, net_by_name["I2C_SDA"], pcbnew.B_Cu, a, b, 0.18)
            router.mark_track("I2C_SDA", 1, a, b, 0.18)

    router.route_net("I2C_SDA", width=0.18)
    router.route_net("I2C_SCL", width=0.18)
    connect_i2c_pullups()

    # Reset pushbutton branch on B.Cu, tied into the R3/C7 reset node.
    nr_net = net_by_name["NRST"]
    nr_r = next(p for p in pad_terms["NRST"] if p.GetParentFootprint().GetReference() == "R3")
    nr_c = next(p for p in pad_terms["NRST"] if p.GetParentFootprint().GetReference() == "C7")
    nr_sw = next(p for p in pad_terms["NRST"] if p.GetParentFootprint().GetReference() == "SW4")
    rp, cp, sp = nr_r.GetPosition(), nr_c.GetPosition(), nr_sw.GetPosition()
    rxy = (pcbnew.ToMM(rp.x), pcbnew.ToMM(rp.y))
    cxy = (pcbnew.ToMM(cp.x), pcbnew.ToMM(cp.y))
    sxy = (pcbnew.ToMM(sp.x), pcbnew.ToMM(sp.y))
    add_track(board, nr_net, pcbnew.F_Cu, rxy, cxy, 0.18)
    router.mark_track("NRST", 0, rxy, cxy, 0.18)
    nr_via = router.world(*router.grid(rxy[0], rxy[1] + 2.0))
    add_track(board, nr_net, pcbnew.F_Cu, rxy, nr_via, 0.18)
    router.mark_track("NRST", 0, rxy, nr_via, 0.18)
    add_via(board, nr_net, nr_via)
    router._mark_via("NRST", router.grid(*nr_via))
    for a, b in [(nr_via, (nr_via[0], sxy[1])), ((nr_via[0], sxy[1]), sxy)]:
        add_track(board, nr_net, pcbnew.B_Cu, a, b, 0.18)
        router.mark_track("NRST", 1, a, b, 0.18)
    pad_terms["NRST"] = [p for p in pad_terms["NRST"]
                          if p.GetParentFootprint().GetReference() not in {"SW4", "C7"}]
    router.route_net("+5V", width=0.3)
    router.connect_ground_smd()
    router.route_net("BOOT0", width=0.18)
    for name in ("SWDIO", "SWCLK", "DS18B20_DQ", "NRST", "KEY1", "KEY2", "KEY3"):
        router.route_net(name, width=0.18)
    router.route_net("+3V3", width=0.18)

    # Route power first, followed by analog/clock buses and low-speed GPIO.
    order = [
        "PT100_ADC", "PT100_RAW",
        "UART2_TX_ESP_RX", "UART2_RX_ESP_TX", "DEBUG_TX", "DEBUG_RX",
        "ESP_GPIO2", "ESP_GPIO0", "ESP_EN", "ESP_RST",
        "BUZZER", "BUZZER_BASE", "BUZZER_COL", "ALARM_LED", "ALARM_LED_A", "POWER_LED_A",
    ]
    for name in order:
        if name in pad_terms:
            router.route_net(name, width=0.6 if name in {"USB_5V", "+5V"} else 0.18)

    add_zone(board, net_by_name["GND"], pcbnew.In1_Cu)
    add_zone(board, net_by_name["+3V3"], pcbnew.In2_Cu)
    add_zone(board, net_by_name["GND"], pcbnew.B_Cu)

    add_text(board, "TEMPERATURE INSPECTOR  STM32F103CBT6", (55, 6.8), 1.2)
    add_text(board, "USB-C 5V POWER ONLY", (16, 32), 1.0)
    add_text(board, "PT100 VOUT < 3.3V", (25, 37), 0.9)
    add_text(board, "DS18B20", (10, 38), 0.8)
    add_text(board, "PT100", (18, 38), 0.8)
    add_text(board, "OLED", (27, 38), 0.8)
    add_text(board, "MICRO-SD", (65, 38), 0.8)
    add_text(board, "UART1", (92, 41), 0.8)
    add_text(board, "SWD", (100, 41), 0.8)
    add_text(board, "KEY1", (75, 75), 0.8)
    add_text(board, "KEY2", (62, 75), 0.8)
    add_text(board, "KEY3", (49, 75), 0.8)
    add_text(board, "Rev 1.0  2026-07-15", (82, 82), 0.8)
    add_text(board, "OPEN HARDWARE - jiayu-here", (55, 82), 0.8, pcbnew.B_SilkS)

    pcbnew.SaveBoard(str(PROJECT / f"{NAME}.kicad_pcb"), board)


def main():
    PROJECT.mkdir(parents=True, exist_ok=True)
    write_legacy_schematic()
    write_board()
    pro = PROJECT / f"{NAME}.kicad_pro"
    if not pro.exists():
        pro.write_text("{}\n", encoding="utf-8")
    print(f"Generated {NAME}.sch and {NAME}.kicad_pcb in {PROJECT}")


if __name__ == "__main__":
    main()
