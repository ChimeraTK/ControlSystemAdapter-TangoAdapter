#!/usr/bin/python3

# SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
# SPDX-License-Identifier: LGPL-3.0-or-later

import sys
import lxml.etree as ET
import argparse
import typing

ns = {'ac': 'https://github.com/ChimeraTK/ApplicationCore'}

data_map = {
        'uint64': 'DevULong64',
        'int64': 'DevLong64',
        'uint32': 'DevULong',
        'int32': 'DevLong',
        'uint16': 'DevUShort',
        'int16': 'DevShort',
        'uint8': 'DevUChar',
        'int8': 'DevChar',
        'float': 'DevFloat',
        'double': 'DevDouble',
        'string' : 'DevString',
        'Boolean' : 'DevBoolean',
        'Void' : 'DevVoid'
}

def create_attribute_list(path: str, output: typing.TextIO) -> None:
    root = ET.parse(path)
    application = root.xpath("/ac:application", namespaces=ns)[0]

    for elem in root.xpath("//ac:variable", namespaces=ns):
        p = elem
        path = ""
        while p is not application:
            path = p.get("name") + "/" + path
            p = p.getparent()
        path=path[0:-1]

        value_type = data_map[elem.xpath("./ac:value_type/text()", namespaces=ns)[0]]
        elements = int(elem.xpath("./ac:numberOfElements/text()", namespaces=ns)[0])
        if elements > 1:
            form="SPECTRUM"
        else:
            form="SCALAR"
        description = elem.xpath("./ac:description/text()", namespaces=ns)
        if len(description) == 0:
            description = ""
        else:
            description = description[0]
        unit = elem.xpath("./ac:unit/text()", namespaces=ns)
        if len(unit) == 0:
            unit="unknown"
        else:
            unit = unit[0]

        print(";".join((path.replace("/", "_"), path, form, value_type, description, unit)), file=output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="xml2tango",
                                     description="Generate Tango adapter attribute list from ApplicationCore server XML dump")
    parser.add_argument('filename')
    parser.add_argument('-o', '--output', type=argparse.FileType('w', encoding="UTF-8"), default=sys.stdout)

    args = parser.parse_args()
    create_attribute_list(args.filename, args.output)
