import re

class Warning:
    name: str
    description: str
    id: int
    enabled: bool

    def __init__(self, name, description, id, enabled = True):
        self.name = name
        self.description = description
        self.id = id
        self.enabled = enabled

    def __str__(self):
        return f'<tr><td><code>W{self.id}</code></td><td>{self.name}</td><td>{self.description}</td><td>{self.enabled}</td></tr>\n'

class Errors:
    name: str
    description: str
    id: int

    def __init__(self, name, description, id):
        self.name = name
        self.description = description
        self.id = id
    
    def __str__(self):
        return f'<tr><td><code>E{self.id}</code></td><td>{self.name}</td><td>{self.description}</td></tr>\n'



with open('src/asar/errors.cpp', 'r') as f:
    errors_text = f.readlines()

with open('src/asar/warnings.cpp', 'r') as f:
    warnings_text = f.readlines()

warnings: list[Warning] = []
errors: list[Errors] = []

warning_lines = []
error_lines = []
for line in warnings_text:
    if line.strip().startswith('{ WRN('):
        warning_lines.append(line.strip())
for line in errors_text:
    if line.strip().startswith('{ error_id_'):
        error_lines.append(line.strip())

warn_id = 1001
err_id = 5001

pre_error_text = """
<!DOCTYPE html>
<html lang="en">
  <head>
    <title>Asar User Manual</title>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <link rel="stylesheet" href="../shared/highlight_js/styles/default.css" />
    <script src="../shared/highlight_js/highlight.pack.js"></script>
    <script>
      hljs.configure({
        tabReplace: "    ",
      });
      hljs.initHighlightingOnLoad();

      function toggle_visibility(id) {
        var e = document.getElementById(id);
        var label = document.getElementById(id.concat("-toggle"));

        if (e.style.display == "none") {
          label.innerHTML = label.innerHTML.replace(
            "[+] Expand",
            "[-] Collapse"
          );
          e.style.display = "block";
        } else {
          label.innerHTML = label.innerHTML.replace(
            "[-] Collapse",
            "[+] Expand"
          );
          e.style.display = "none";
        }
      }
    </script>
  </head>
  <body>
    <table>
      <tr>
        <th style="width: 150px">Error ID</th>
        <th>Internal name</th>
        <th>Error message</th>
      </tr>
"""
pre_warning_text = """
<!DOCTYPE html>
<html lang="en">
  <head>
    <title>Asar User Manual</title>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <link rel="stylesheet" href="../shared/highlight_js/styles/default.css" />
    <script src="../shared/highlight_js/highlight.pack.js"></script>
    <script>
      hljs.configure({
        tabReplace: "    ",
      });
      hljs.initHighlightingOnLoad();

      function toggle_visibility(id) {
        var e = document.getElementById(id);
        var label = document.getElementById(id.concat("-toggle"));

        if (e.style.display == "none") {
          label.innerHTML = label.innerHTML.replace(
            "[+] Expand",
            "[-] Collapse"
          );
          e.style.display = "block";
        } else {
          label.innerHTML = label.innerHTML.replace(
            "[-] Collapse",
            "[+] Expand"
          );
          e.style.display = "none";
        }
      }
    </script>
  </head>
  <body>
    <table>
      <tr>
        <th style="width: 150px">Warning ID</th>
        <th>Internal name</th>
        <th>Warning message</th>
        <th>Enabled by default</th>
      </tr>
"""
post_text = """
    </table>
  </body>
</html>
"""

warning_pattern = re.compile(r'WRN\((.*)\),\s*"(.*)"\s*,?\s*(true|false)?\s*}')
error_pattern = re.compile(r'{\s*error_id_(.*),\s*"(.*)"\s*}')
for warning in warning_lines:
    name, description, enabled = re.findall(warning_pattern, warning)[0]
    warnings.append(Warning(name, description, warn_id, enabled != 'false'))
    warn_id += 1

for error in error_lines:
    name, description = re.findall(error_pattern, error)[0]
    errors.append(Errors(name, description, err_id))
    err_id += 1

with open('docs/manual/errors-list.html', 'w') as f:
    f.write(pre_error_text)
    for error in errors:
        f.write(str(error))
    f.write(post_text)

with open('docs/manual/warnings-list.html', 'w') as f:
    f.write(pre_warning_text)
    for warning in warnings:
        f.write(str(warning))
    f.write(post_text)