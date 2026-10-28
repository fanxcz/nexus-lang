#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        int commandExport(const std::vector<std::string>& args) {
            const auto cfg = loadProject();
            std::string target = cfg.target;
            std::string backend = "auto";
            std::string explicitOutput;
            for (std::size_t i = 0; i < args.size(); ++i) {
                const std::string& a = args[i];
                if (a.rfind("--target=", 0) == 0)
                    target = a.substr(9);
                else if (a == "--target" && i + 1 < args.size())
                    target = args[++i];
                else if (a.rfind("--backend=", 0) == 0)
                    backend = a.substr(10);
                else if (a == "--backend" && i + 1 < args.size())
                    backend = args[++i];
                else if (a == "-o" && i + 1 < args.size())
                    explicitOutput = args[++i];
                else
                    throw std::runtime_error("unknown option: " + a);
            }
            const auto in = cfg.root / cfg.entry;
            const std::string resolvedTriple = targetTriple(target);
            bool windowsTarget = false;
#ifdef _WIN32
            windowsTarget = target.empty() || target == "native" ||
                            resolvedTriple.find("windows") != std::string::npos;
#else
            windowsTarget = resolvedTriple.find("windows") != std::string::npos;
#endif
            const auto dist = cfg.root / "dist";
            fs::create_directories(dist);
            fs::path out;
            if (!explicitOutput.empty()) {
                out = fs::path(explicitOutput);
                if (out.is_relative())
                    out = dist / out;
            } else {
                out = dist / cfg.artifact;
            }
            if (windowsTarget && out.extension().empty())
                out += ".exe";
            compileFile(in, out, target, false, 3, backend);

            const std::string label = target.empty() ? "native" : target;
            const auto stage = dist / (cfg.name + "-" + cfg.version);
            std::error_code ec;
            fs::remove_all(stage, ec);
            fs::create_directories(stage, ec);
            fs::copy(out, stage / out.filename(), fs::copy_options::overwrite_existing, ec);
            if (ec)
                throw std::runtime_error("cannot stage export binary: " + ec.message());
            for (const auto& rel : {std::string("assets"), std::string("scenes")}) {
                if (fs::exists(cfg.root / rel))
                    fs::copy(cfg.root / rel,
                             stage / rel,
                             fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                             ec);
            }
            if (fs::exists(cfg.root / "nexus.toml"))
                fs::copy(cfg.root / "nexus.toml",
                         stage / "nexus.toml",
                         fs::copy_options::overwrite_existing,
                         ec);
            if (ec)
                throw std::runtime_error("cannot stage export files: " + ec.message());
            writeText(stage / "EXPORT.txt",
                      "package = \"" + cfg.name + "\"\nversion = \"" + cfg.version +
                          "\"\ntarget = \"" + label + "\"\nbackend = \"" + backend + "\"\n");

            const auto archive = dist / (cfg.name + "-" + cfg.version + "-" + label + ".zip");
            fs::remove(archive, ec);
            // cmake is only needed to zip the archive; the binary and the
            // staged directory are the actual deliverables. Clean machines
            // without cmake still get a successful export.
            const int rc = runCommand({"cmake",
                                       "-E",
                                       "tar",
                                       "cf",
                                       archive.string(),
                                       "--format=zip",
                                       stage.filename().string()},
                                      dist);
            if (rc == 127) {
                std::cout << "export: binary  -> " << out << "\n"
                          << "export: staged  -> " << stage << "\n"
                          << "export: note: archive skipped (cmake not on PATH)\n";
                return 0;
            }
            if (rc != 0)
                throw std::runtime_error("cannot create export archive");
            std::cout << "export: binary  -> " << out << "\n"
                      << "export: archive -> " << archive << "\n";
            return 0;
        }


        int commandEditor() {

            const auto cfg = loadProject();

            const auto vscode = cfg.root / ".vscode";

            fs::create_directories(vscode);

            writeText(vscode / "settings.json", R"JSON({
  "files.associations": { "*.nx": "nexus" },
  "editor.tabSize": 4,
  "editor.insertSpaces": true
}
)JSON");

            writeText(vscode / "tasks.json", R"JSON({
  "version": "2.0.0",
  "tasks": [{
    "label": "NEXUS Build",
    "type": "shell",
    "command": "nexus build --release",
    "group": "build"
  }]
}
)JSON");

            std::cout << "editor: generated VS Code integration in .vscode/\n";

            std::cout << "editor: use `nexus lsp` as the language-server command\n";

            return 0;
        }

    } // namespace driver
} // namespace nexus
