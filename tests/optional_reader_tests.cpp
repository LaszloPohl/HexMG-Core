#include "hmgHMGFileReader.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <tuple>

namespace fs = std::filesystem;
using Record = std::tuple<std::string, unsigned, unsigned, std::string>;
static void check(bool good, const char* message) { if (!good) throw std::runtime_error(message); }
static void write(const fs::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary);
    file << text; file.close(); check(bool(file), "Fixture write failed");
}
static std::vector<Record> read(const fs::path& path) {
    nsHMG::ReadALine reader;
    check(reader.open(path.parent_path().generic_string() + "/", path.filename().string()), "Reader open failed");
    std::vector<Record> records;
    char line[MAX_LINE_LENGHT]; nsHMG::LineInfo info{};
    while (reader.getLine(line, sizeof(line), info))
        records.emplace_back(line, info.firstLine, info.lastLine, reader.getFileName(info));
    return records;
}
int main(int argc, char** argv) {
    try {
        check(argc == 2, "Provide a fresh fixture directory");
        const fs::path root = fs::absolute(argv[1]);
        check(!fs::exists(root) && fs::create_directories(root), "Fixture directory must be new");
        unsigned count = 0;
        const auto test = [&](const std::string& name, const std::string& input,
                              const std::vector<Record>& expected) {
            write(root / name, input);
            const auto actual = read(root / name);
            if (actual != expected) {
                for (const auto& [text, first, last, file] : actual)
                    std::cerr << file << ':' << first << '-' << last << " [" << text << "]\n";
            }
            check(actual == expected, name.c_str()); ++count;
        };
        test("empty.hmg", ".OPTIONAL\n.END OPTIONAL\nA\n", {{"A",3,3,"empty.hmg"}});
        test("opaque.hmg", ".OPTIONAL any strange header\n.INCLUDE missing\n.RUN impossible\n.END\n+ anything\n.END OPTIONAL\nA\n",
            {{"A",7,7,"opaque.hmg"}});
        test("nested.hmg", ".OPTIONAL\n.OPTIONAL ignored too\n.END OPTIONAL\nA\n", {{"A",4,4,"nested.hmg"}});
        test("case.hmg", ".optional stuff\r\nignored\r\n.end\t optional ; comment\r\na\r\n", {{"A",4,4,"case.hmg"}});
        test("lf-cr.hmg", ".OPTIONAL\n\rignored\n\r.END OPTIONAL\n\rA\n\r", {{"A",4,4,"lf-cr.hmg"}});
        test("no-final-newline.hmg", "A\n.OPTIONAL\n.END OPTIONAL", {{"A",1,1,"no-final-newline.hmg"}});
        test("comments.hmg", "; .OPTIONAL\nA ; x\n.OPTIONAL\n; .END OPTIONAL\n\".END OPTIONAL\"\n.END OPTIONALX\n.E'N'D OPTIONAL\n.END OPTIONAL extra\n.END OPTIONAL // done\nB\n",
            {{"A",2,2,"comments.hmg"},{"B",10,10,"comments.hmg"}});
        test("continuation.hmg", "A\n.OPTIONAL\n+ ignored\n.END OPTIONAL\n+B\nC\n",
            {{"AB",1,5,"continuation.hmg"},{"C",6,6,"continuation.hmg"}});
        test("multiple.hmg", ".OPTIONAL\n.END OPTIONAL\n.OPTIONAL X\n.END OPTIONAL\nA\n", {{"A",5,5,"multiple.hmg"}});
        test("similar-name.hmg", ".OPTIONALLY X\nA\n", {{".OPTIONALLY X",1,1,"similar-name.hmg"},{"A",2,2,"similar-name.hmg"}});
        test("contexts.hmg", ".MODEL M\n.OPTIONAL X\ninvalid\n.END OPTIONAL\nR1 2\n.END MODEL M\n",
            {{".MODEL M",1,1,"contexts.hmg"},{"R1 2",5,5,"contexts.hmg"},{".END MODEL M",6,6,"contexts.hmg"}});
        test("function.hmg", ".FUNCTION F\n.OPTIONAL anything\n.END FUNCTION F\n.END OPTIONAL\nRETURN 1\n.END FUNCTION F\n",
            {{".FUNCTION F",1,1,"function.hmg"},{"RETURN 1",5,5,"function.hmg"},{".END FUNCTION F",6,6,"function.hmg"}});
        test("digits.hmg", "R 1'000\n.OPTIONAL\n.END OPTIONAL\nA\n", {{"R 1000",1,1,"digits.hmg"},{"A",4,4,"digits.hmg"}});
        test("long-body.hmg", ".OPTIONAL\n" + std::string(1500000,'x') + "\n.END OPTIONAL\nA\n",
            {{"A",4,4,"long-body.hmg"}});
        for (unsigned distance = 0; distance < 12; ++distance) {
            const auto name = "buffer" + std::to_string(distance) + ".hmg";
            // Place the opening directive across the existing 512 KiB buffers.
            const auto padding = std::string(524288 - distance - 2, 'x');
            test(name, ";" + padding + "\n.OPTIONAL\nignored\n.END OPTIONAL\nA\n", {{"A",5,5,name}});
            const auto endName = "end-buffer" + std::to_string(distance) + ".hmg";
            test(endName, ".OPTIONAL\n" + std::string(524288 - distance - 11, 'x') + "\n.END OPTIONAL\nA\n",
                {{"A",4,4,endName}});
        }
        write(root / "child.hmg", "C\n.OPTIONAL\n.INCLUDE missing\n.END OPTIONAL\nD\n");
        // The baseline reader already reports the parent line after an LF-only
        // INCLUDE one line early; OPTIONAL must not change that legacy behavior.
        test("parent.hmg", ".INCLUDE child.hmg\nP\n", {{"C",1,1,"child.hmg"},{"D",5,5,"child.hmg"},{"P",1,1,"parent.hmg"}});
        const auto reject = [&](const std::string& name, const std::string& input, const std::string& file, unsigned line) {
            write(root / name, input); bool rejected = false;
            try { (void)read(root / name); }
            catch (const nsHMG::hmgExcept& error) {
                const std::string message = error.what();
                rejected = message.find("missing .END OPTIONAL") != std::string::npos &&
                    message.find(file) != std::string::npos && message.find("line " + std::to_string(line)) != std::string::npos;
            }
            check(rejected, name.c_str()); ++count;
        };
        reject("missing.hmg", "A\n.OPTIONAL\nignored\n", "missing.hmg", 2);
        reject("header-eof.hmg", ".OPTIONAL", "header-eof.hmg", 1);
        write(root / "bad-child.hmg", ".OPTIONAL\nignored\n");
        reject("cross-file.hmg", ".INCLUDE bad-child.hmg\n.END OPTIONAL\n", "bad-child.hmg", 1);
        std::cout << count << " OPTIONAL reader scenarios PASS\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
