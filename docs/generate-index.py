import json
import sys
import re

# don't clutter the book directory with __pycache__
sys.dont_write_bytecode = True

import parse_warnings

index = {}

class Location():
    def __init__(self, path, name, anchor):
        self.path = path
        self.name = name
        self.anchor = anchor

def walk_section(sec):
    # recursively find and replace all matches of {{# stuff #}}
    if "Chapter" in sec:
        chap = sec["Chapter"]
        for x in chap["sub_items"]:
            walk_section(x)
        anchor = 0
        def replace_match(m):
            nonlocal anchor
            content = m.group(1).strip()
            this_anch = "a" + str(anchor)
            anchor += 1
            if content.startswith("cmd:"):
                cmd_name = content.removeprefix("cmd:").strip()
                index[cmd_name] = Location(chap["path"], chap["name"], this_anch)
                return f'<a name="{this_anch}"></a>`{cmd_name}`'
            elif content.startswith("hiddencmd:"):
                cmd_name = content.removeprefix("hiddencmd:").strip()
                index[cmd_name] = Location(chap["path"], chap["name"], this_anch)
                return f'<a name="{this_anch}"></a>'
            elif content.startswith("syn:"):
                syn = content.removeprefix("syn:").strip().split("\n")
                output = f'<a name="{this_anch}"></a>\n```asar\n'
                for line in syn:
                    output += line + "\n"
                    index[line] = Location(chap["path"], chap["name"], this_anch)
                return output + '```'
            return content

        chap["content"] = re.sub(r"\{\{#(.*?)#\}\}", replace_match, chap["content"], flags = re.DOTALL)

        if chap["path"] == "cmd-index.md":
            write_index(chap)
        elif chap["path"] == "warning-list.md":
            chap["content"] += "\n".join(parse_warnings.get_warnings())
        elif chap["path"] == "error-list.md":
            chap["content"] += "\n".join(parse_warnings.get_errors())
    return sec

def write_index(chap):
    ind = ""
    for name, entry in index.items():
        entry: Location
        ind += f"`{name}`: [{entry.name}]({entry.path}#{entry.anchor})  \n"
    chap["content"] += ind

def write_warnings(chap):
    chap["content"] += "\n" + "\n".join(parse_warnings.get_warnings())

def write_errors(chap):
    chap["content"] += "\n" + "\n".join(parse_warnings.get_errors())

if __name__ == '__main__':
    if len(sys.argv) > 1: # we check if we received any argument
        if sys.argv[1] == "supports": 
            # then we are good to return an exit status code of 0, since the other argument will just be the renderer's name
            sys.exit(0)

    # load both the context and the book representations from stdin
    context, book = json.load(sys.stdin)
    for x in book['sections']:
        walk_section(x)
    print(json.dumps(book))
