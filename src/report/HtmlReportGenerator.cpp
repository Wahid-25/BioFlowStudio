#include "HtmlReportGenerator.h"

#include <fstream>
#include <stdexcept>

std::string HtmlReportGenerator::escapeHtml(
    const std::string& text
) const
{
    std::string escaped;
    escaped.reserve(text.size());

    for (char character : text)
    {
        switch (character)
        {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped.push_back(character); break;
        }
    }

    return escaped;
}

void HtmlReportGenerator::generate(
    const std::string& filePath,
    const AnalysisReport& report
) const
{
    std::ofstream output(filePath);

    if (!output.is_open())
    {
        throw std::runtime_error("Could not create the HTML report file.");
    }

    output << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>)HTML" << escapeHtml(report.title) << R"HTML(</title>
<style>
body{margin:0;background:#eef3f7;color:#203040;font-family:Arial,sans-serif;line-height:1.5}
.page{max-width:1100px;margin:28px auto;background:white;padding:42px;box-shadow:0 4px 22px #b8c4ce}
h1{color:#163a5f;margin:0 0 6px;font-size:34px}h2{color:#194c73;border-bottom:2px solid #d7e3ec;padding-bottom:7px;margin-top:34px}
.subtitle{font-size:18px;color:#486276}.date{font-size:13px;color:#667b8c;margin-top:8px}.overview{background:#edf7f3;border-left:5px solid #2d6a4f;padding:15px;margin:24px 0}
.metadata{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:10px}.card{background:#f4f7fa;border:1px solid #d7e0e8;border-radius:7px;padding:12px}.card b{display:block;color:#163a5f}
.table-wrap{overflow-x:auto;margin:14px 0 26px}table{width:100%;border-collapse:collapse;font-size:13px}th{background:#163a5f;color:white;text-align:left;padding:9px}td{border:1px solid #d5dee6;padding:8px;vertical-align:top}tr:nth-child(even){background:#f3f7fa}
.figure{border:1px solid #d7e0e8;border-radius:8px;padding:14px;margin:18px 0;text-align:center}.figure img{max-width:100%;height:auto}.figure p{color:#526b7d;font-size:13px}
.note{background:#fff5d9;border-left:5px solid #d79922;padding:11px;margin:9px 0}.footer{margin-top:38px;border-top:1px solid #ccd7df;padding-top:14px;color:#687b89;font-size:12px;text-align:center}
@media print{body{background:white}.page{box-shadow:none;margin:0;max-width:none}.table-wrap{overflow:visible}}
</style>
</head><body><main class="page">
)HTML";

    output << "<h1>" << escapeHtml(report.title) << "</h1>\n";
    output << "<div class=\"subtitle\">"
           << escapeHtml(report.subtitle) << "</div>\n";
    output << "<div class=\"date\">Generated: "
           << escapeHtml(report.generatedAt) << "</div>\n";
    output << "<div class=\"overview\">"
           << escapeHtml(report.overview) << "</div>\n";

    if (!report.metadata.empty())
    {
        output << "<h2>Analysis Overview</h2><div class=\"metadata\">\n";

        for (const auto& item : report.metadata)
        {
            output << "<div class=\"card\"><b>"
                   << escapeHtml(item.first) << "</b>"
                   << escapeHtml(item.second) << "</div>\n";
        }

        output << "</div>\n";
    }

    for (const ReportTable& table : report.tables)
    {
        output << "<h2>" << escapeHtml(table.title) << "</h2>\n";
        output << "<div class=\"table-wrap\"><table><thead><tr>";

        for (const std::string& header : table.headers)
        {
            output << "<th>" << escapeHtml(header) << "</th>";
        }

        output << "</tr></thead><tbody>\n";

        for (const auto& row : table.rows)
        {
            output << "<tr>";

            for (const std::string& value : row)
            {
                output << "<td>" << escapeHtml(value) << "</td>";
            }

            output << "</tr>\n";
        }

        output << "</tbody></table></div>\n";
    }

    if (!report.images.empty())
    {
        output << "<h2>Graphical Results</h2>\n";

        for (const ReportImage& image : report.images)
        {
            output << "<section class=\"figure\"><h3>"
                   << escapeHtml(image.title) << "</h3><img src=\""
                   << escapeHtml(image.relativeFilePath) << "\" alt=\""
                   << escapeHtml(image.title) << "\"><p>"
                   << escapeHtml(image.description) << "</p></section>\n";
        }
    }

    if (!report.notes.empty())
    {
        output << "<h2>Interpretation Notes and Limitations</h2>\n";

        for (const std::string& note : report.notes)
        {
            output << "<div class=\"note\">"
                   << escapeHtml(note) << "</div>\n";
        }
    }

    output << R"HTML(<div class="footer">Generated automatically by BioFlow Studio — Object-Oriented Bioinformatics Workflow Platform</div>
</main></body></html>)HTML";
}
