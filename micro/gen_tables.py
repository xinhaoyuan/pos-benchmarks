#!/usr/bin/env python

import sys, os
import re
import math

table1 = {
    "columns" : [
        ("RW", 0),
        ("PCT", 1),
        ("RAPOS", 2),
        ("\myname{}", 4),
        ("\mynamepo{}", 6),
        # ("HPORPS", 8)
    ],
    "rows" : [
        ("Uniform", [
            "paper-micro-bench/uniform_0.result",
            "paper-micro-bench/uniform_1.result",
            "paper-micro-bench/uniform_2.result",
            "paper-micro-bench/uniform_3.result",
            "paper-micro-bench/uniform_4.result",
        ]),

        ("Skewed", [
            "paper-micro-bench/skewed_0.result",
            "paper-micro-bench/skewed_1.result",
            "paper-micro-bench/skewed_2.result",
            "paper-micro-bench/skewed_3.result",
            "paper-micro-bench/skewed_4.result",
        ]),
    ]
}

table2 = {
    "columns" : [
        ("RW", 0),
        ("PCT", 1),
        ("RAPOS", 2),
        ("\myname{}", 4),
        ("\mynamepo{}", 7),
        ("\mynamepoext{}", 6),
        # ("HPORPS", 8)
    ],
    "rows" : [
        ("Uniform", [
            "paper-micro-bench/uniform-rw_0.result",
            "paper-micro-bench/uniform-rw_1.result",
            "paper-micro-bench/uniform-rw_2.result",
            "paper-micro-bench/uniform-rw_3.result",
            "paper-micro-bench/uniform-rw_4.result",
        ]),

        ("Skewed", [
            "paper-micro-bench/skewed-rw_0.result",
            "paper-micro-bench/skewed-rw_1.result",
            "paper-micro-bench/skewed-rw_2.result",
            "paper-micro-bench/skewed-rw_3.result",
            "paper-micro-bench/skewed-rw_4.result",
        ]),
    ]
}

num_trials = int(os.environ.get("NUM_TRIALS", "50000000"))

def make_table(table_data, out):
    out.write("\\begin{tabular}{c|r|r|r|" + ("|r" * len(table_data["columns"])) + "}\n")
    out.write("\\multicolumn{4}{c||}{Benchmark} & \\multicolumn{" + str(len(table_data["columns"])) + "}{c}{Coverage}\\\\\\hline\n")
    out.write("Conf. & \\addstackgap{\shortstack{PO.\\\\count}} & \shortstack{Max\\\\prempt.} & \shortstack{Max\\\\races} ")
    for col in table_data["columns"]:
        n = col[0]
        out.write("& " + n)
    out.write("\\\\\n")
    out.write("\\hline\n")
    geo = {}
    case_no = 0
    for r in range(len(table_data["rows"])):
        out.write("\\hline\n")
        row = table_data["rows"][r]
        for i in range(len(row[1])):
            if i == 0:
                out.write("\multirow{" + str(len(row[1])) + "}{*}{" + row[0] + "}")
            out.write("&")
            case_no += 1
            # out.write(str(case_no) + "&")

            prog_size = None
            min_preemptions = None
            coverage = None
            min = None
            with open(row[1][i]) as data:
                for line in data:
                    m = re.search("Total PO traces: ([0-9]+)", line)
                    if m:
                        prog_size = int(m.group(1))
                        continue
                    m = re.search("Max Preemptions: ([0-9]+)", line)
                    if m:
                        min_preemptions = int(m.group(1))
                        continue
                    m = re.search("Max Races: ([0-9]+)", line)
                    if m:
                        max_races = int(m.group(1))
                        continue
                    if line.startswith("Coverage,"):
                        coverage_raw = line.strip().split(",")[1:]
                        coverage = {}
                        for col in table_data["columns"]:
                            coverage[col[0]] = int(coverage_raw[col[1]])
                    if line.startswith("Min,"):
                        min_raw = line.strip().split(",")[1:]
                        min = {}
                        for col in table_data["columns"]:
                            min[col[0]] = float(min_raw[col[1]])

            out.write(str(prog_size) + "&" + str(min_preemptions) + "&" + str(max_races))
            for col in table_data["columns"]:
                n = col[0]
                if coverage[n] == prog_size:
                    geo_v = math.log(min[n])
                    out.write("&{:.2e}".format(min[n]))
                    # out.write("&{:.03f}".format(min[n] * prog_size))
                else:
                    out.write("&0({0})".format(coverage[n]))
                    geo_v = -math.log(num_trials)

                if n not in geo or geo_v is None:
                    geo[n] = geo_v
                elif geo[n] is not None:
                    geo[n] += geo_v

            out.write("\\\\\n")
    out.write("\\hline\\hline\n")
    out.write("\\multicolumn{4}{c||}{\\bf Geo-mean*}")
    for col in table_data["columns"]:
        n = col[0]
        if n not in geo or geo[n] is None:
            out.write("&0")
        else:
            out.write("&{:.2e}".format(math.exp(geo[n] / case_no)))
    out.write("\\end{tabular}\n")

def make_html_table(table_data, out):
    out.write("""
<style>
td.tab-header { border: 1px solid black; }
td.tab-data { border: 1px solid black; }
</style>
""")
    out.write("<table>")
    out.write("<tr><td class=\"tab-header\" align=\"center\" colspan=\"4\">Benchmark</td><td class=\"tab-header\" align=\"center\" colspan=\"" + str(len(table_data["columns"])) + "\">Coverage</td></tr>\n")
    out.write("<tr><td class=\"tab-header\" align=\"center\">Conf.</td><td class=\"tab-header\" align=\"center\">PO. count</td><td class=\"tab-header\" align=\"center\">Max prempt.</td><td class=\"tab-header\" align=\"center\">Max races</td>")
    for col in table_data["columns"]:
        n = col[0]
        if n == "\\myname{}":
            n = "BasicPOS"
        elif n == "\\mynamepo{}":
            n = "POS"
        elif n == "\\mynamepoext{}":
            n = "POS*"
        out.write("<td class=\"tab-header\" align=\"center\">" + n + "</td>")
    out.write("</tr>\n")
    geo = {}
    case_no = 0
    for r in range(len(table_data["rows"])):
        out.write("<tr>")
        row = table_data["rows"][r]
        for i in range(len(row[1])):
            if i == 0:
                out.write("<td class=\"tab-header\" align=\"center\" rowspan=\"" + str(len(row[1])) + "\">" + row[0] + "</td>")
            case_no += 1
            # out.write(str(case_no) + "&")

            prog_size = None
            min_preemptions = None
            coverage = None
            min = None
            with open(row[1][i]) as data:
                for line in data:
                    m = re.search("Total PO traces: ([0-9]+)", line)
                    if m:
                        prog_size = int(m.group(1))
                        continue
                    m = re.search("Max Preemptions: ([0-9]+)", line)
                    if m:
                        min_preemptions = int(m.group(1))
                        continue
                    m = re.search("Max Races: ([0-9]+)", line)
                    if m:
                        max_races = int(m.group(1))
                        continue
                    if line.startswith("Coverage,"):
                        coverage_raw = line.strip().split(",")[1:]
                        coverage = {}
                        for col in table_data["columns"]:
                            coverage[col[0]] = int(coverage_raw[col[1]])
                    if line.startswith("Min,"):
                        min_raw = line.strip().split(",")[1:]
                        min = {}
                        for col in table_data["columns"]:
                            min[col[0]] = float(min_raw[col[1]])

            out.write("<td class=\"tab-data\" align=\"right\">{0}</td><td class=\"tab-data\" align=\"right\">{1}</td><td class=\"tab-data\" align=\"right\">{2}</td>".format(prog_size, min_preemptions, max_races))
            for col in table_data["columns"]:
                n = col[0]
                if coverage[n] == prog_size:
                    geo_v = math.log(min[n])
                    out.write("<td class=\"tab-data\" align=\"right\">{:.2e}</td>".format(min[n]))
                    # out.write("&{:.03f}".format(min[n] * prog_size))
                else:
                    out.write("<td class=\"tab-data\" align=\"right\">0({0})</td>".format(coverage[n]))
                    geo_v = -math.log(num_trials)

                if n not in geo or geo_v is None:
                    geo[n] = geo_v
                elif geo[n] is not None:
                    geo[n] += geo_v

            out.write("</tr>\n")
    out.write("<tr><td class=\"tab-header\" align=\"center\" colspan=\"4\">Geo-mean*</td>")
    for col in table_data["columns"]:
        n = col[0]
        if n not in geo or geo[n] is None:
            out.write("<td class=\"tab-data\" align=\"right\">0</td>")
        else:
            out.write("<td class=\"tab-data\" align=\"right\">{:.2e}</td>".format(math.exp(geo[n] / case_no)))
    out.write("</tr>\n</table>\n")


make_table(table1, open("table-micro-1.tex", "w"))
make_html_table(table1, open("table-micro-1.html", "w"))
make_table(table2, open("table-micro-2.tex", "w"))
make_html_table(table2, open("table-micro-2.html", "w"))
