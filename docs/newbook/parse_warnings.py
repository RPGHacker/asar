# actually parses errors too
import re
import string

warning_pattern = re.compile(r'WRN\((.*?),\s*"(.*)"\s*,\s*(true|false)\)')
error_pattern = re.compile(r'ERR\((.*?),\s*"(.*)"\)')
escaping = str.maketrans({x: '\\'+x for x in string.punctuation})

def escape(s):
    s = s.replace('\\"', '"')
    return s.translate(escaping)

def get_errors():
    yield "| Error name | Message |"
    yield "| ---------- | ------- |"
    with open("../../src/asar/errors.h") as f:
        for line in f:
            if line.startswith('ERR('):
                name, description = re.findall(error_pattern, line)[0]
                yield f"| E{escape(name)} | {escape(description)} |"

def get_warnings():
    yield "| Warning name | Message | Enabled by default |"
    yield "| ------------ | ------- | ------------------ |"
    with open("../../src/asar/warnings.h") as f:
        for line in f:
            if line.startswith('WRN('):
                name, description, enabled = re.findall(warning_pattern, line)[0]
                enabled = "no" if enabled == "false" else "yes"
                yield f"| W{escape(name)} | {escape(description)} | {enabled} |"
