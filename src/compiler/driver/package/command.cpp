#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        int commandPkg(const std::vector<std::string>& args) {
            const auto cfg = loadProject();
            if (args.empty() || args[0] == "help") {
                std::cout
                    << "nexus pkg init|add <path>|remove <name>|list|update|verify|build|publish\n";
                return 0;
            }
            const std::string sub = args[0];
            const fs::path vendor = cfg.root / "vendor";
            if (sub == "init" || sub == "add" || sub == "remove")
                ensureSafeVendorDirectory(cfg, true);
            else if (sub == "list" || sub == "update" || sub == "verify")
                ensureSafeVendorDirectory(cfg, false);
            if (sub == "init") {
                fs::create_directories(vendor);
                refreshLockfile(cfg);
                std::cout << "pkg init: OK\n";
                return 0;
            }
            if (sub == "list") {
                std::cout << "Packages for " << cfg.name << ":\n";
                if (fs::exists(vendor)) {
                    for (const auto& entry : fs::directory_iterator(vendor)) {
                        if (!entry.is_directory())
                            continue;
                        std::cout << "  " << entry.path().filename().string();
                        if (fs::is_regular_file(entry.path() / "nexus.toml")) {
                            try {
                                std::cout << " " << loadProject(entry.path()).version;
                            } catch (const std::exception&) {
                                std::cout << " (invalid manifest)";
                            }
                        }
                        std::cout << '\n';
                    }
                }
                return 0;
            }
            if (sub == "add") {
                if (args.size() != 2)
                    throw std::runtime_error(
                        "pkg add requires exactly one local package directory");
                fs::path source(args[1]);
                if (source.is_relative())
                    source = fs::absolute(source);
                std::error_code ec;
                source = fs::weakly_canonical(source, ec);
                if (ec || !fs::is_directory(source, ec))
                    throw std::runtime_error("pkg add source is not an accessible directory");
                if (!fs::is_regular_file(source / "nexus.toml"))
                    throw std::runtime_error("package directory must contain nexus.toml");
                const auto dep = loadProject(source);
                if (!validPackageName(dep.name))
                    throw std::runtime_error(
                        "invalid package name: use letters, digits, '_' or '-'");
                const fs::path root = fs::weakly_canonical(cfg.root, ec);
                if (ec)
                    throw std::runtime_error("cannot resolve project root: " + ec.message());
                const fs::path destination = (vendor / dep.name).lexically_normal();
                if (source == root)
                    throw std::runtime_error(
                        "cannot add the current project as its own dependency");
                const fs::path relativeDestination = destination.lexically_relative(source);
                if (!relativeDestination.empty() && *relativeDestination.begin() != "..")
                    throw std::runtime_error("package source contains the vendor destination; "
                                             "choose a separate directory");

                const fs::path manifestPath = cfg.root / "nexus.toml";
                const fs::path lockPath = cfg.root / "nexus.lock";
                const std::string manifestBefore = readFile(manifestPath);
                const auto lockStatus = fs::symlink_status(lockPath, ec);
                bool hadLockfile = false;
                if (ec == std::errc::no_such_file_or_directory ||
                    (!ec && lockStatus.type() == fs::file_type::not_found)) {
                    ec.clear();
                } else {
                    if (ec)
                        throw std::runtime_error("cannot inspect lockfile: " + ec.message());
                    if (fs::is_symlink(lockStatus) || !fs::is_regular_file(lockStatus))
                        throw std::runtime_error(
                            "nexus.lock must be a regular file, not a symlink");
                    hadLockfile = true;
                }
                const std::string lockBefore = hadLockfile ? readFile(lockPath) : std::string{};

                const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
                const fs::path backup =
                    root / (".nexus-pkg-backup-" + dep.name + "-" + std::to_string(stamp));
                if (fs::exists(backup, ec) || ec)
                    throw std::runtime_error("cannot reserve package rollback path");
                ec.clear();

                const auto destinationStatus = fs::symlink_status(destination, ec);
                const bool hadDestination =
                    !ec && destinationStatus.type() != fs::file_type::not_found;
                if (ec == std::errc::no_such_file_or_directory)
                    ec.clear();
                else if (ec)
                    throw std::runtime_error("cannot inspect existing vendored package: " +
                                             ec.message());
                if (hadDestination) {
                    fs::rename(destination, backup, ec);
                    if (ec)
                        throw std::runtime_error("cannot stage existing vendored package: " +
                                                 ec.message());
                }

                try {
                    copyPackageTree(source, destination);
                    updateDependency(manifestPath, dep.name, "vendor/" + dep.name, false);
                    refreshLockfile(cfg);
                } catch (...) {
                    std::string rollbackFailure;
                    try {
                        std::error_code rollbackError;
                        fs::remove_all(destination, rollbackError);
                        if (rollbackError)
                            throw std::runtime_error("cannot remove failed package copy: " +
                                                     rollbackError.message());
                        if (hadDestination) {
                            fs::rename(backup, destination, rollbackError);
                            if (rollbackError)
                                throw std::runtime_error("cannot restore previous package: " +
                                                         rollbackError.message());
                        }
                        writeTextAtomically(manifestPath, manifestBefore);
                        if (hadLockfile) {
                            writeTextAtomically(lockPath, lockBefore);
                        } else {
                            fs::remove(lockPath, rollbackError);
                            if (rollbackError)
                                throw std::runtime_error("cannot remove failed lockfile: " +
                                                         rollbackError.message());
                        }
                    } catch (const std::exception& rollbackError) {
                        rollbackFailure = rollbackError.what();
                    }
                    if (!rollbackFailure.empty())
                        throw std::runtime_error("pkg add failed and rollback was incomplete: " +
                                                 rollbackFailure);
                    throw;
                }

                if (hadDestination) {
                    fs::remove_all(backup, ec);
                    if (ec)
                        std::cerr << "pkg add: warning: old package backup remains at " << backup
                                  << ": " << ec.message() << '\n';
                }
                std::cout << "pkg add: vendored " << dep.name << " " << dep.version << "\n";
                return 0;
            }
            if (sub == "remove") {
                if (args.size() != 2)
                    throw std::runtime_error("pkg remove requires exactly one package name");
                const std::string name = args[1];
                if (!validPackageName(name))
                    throw std::runtime_error(
                        "invalid package name: path separators and traversal are not allowed");
                std::error_code ec;
                fs::remove_all(vendor / name, ec);
                if (ec)
                    throw std::runtime_error("cannot remove vendored package: " + ec.message());
                updateDependency(cfg.root / "nexus.toml", name, "", true);
                refreshLockfile(cfg);
                std::cout << "pkg remove: removed " << name
                          << " and refreshed nexus.toml/nexus.lock\n";
                return 0;
            }
            if (sub == "update") {
                refreshLockfile(cfg);
                std::cout << "pkg update: lockfile refreshed from nexus.toml\n";
                return 0;
            }
            if (sub == "verify") {
                const std::string manifest = readFile(cfg.root / "nexus.toml");
                const std::string lock = readFile(cfg.root / "nexus.lock");
                const auto manifestDependencies =
                    readStrictTomlStringSection(manifest, "dependencies", "nexus.toml");
                const auto parsedLock = parsePackageLockfile(lock);
                static const std::map<std::string, std::string> emptyLockSection;
                const auto lockSection =
                    [&](const std::string& name) -> const std::map<std::string, std::string>& {
                    const auto section = parsedLock.sections.find(name);
                    return section == parsedLock.sections.end() ? emptyLockSection
                                                                : section->second;
                };
                const auto& lockedDependencies = lockSection("dependencies");
                const auto& sources = lockSection("sources");
                const auto& integrities = lockSection("integrity");

                bool failed = false;
                std::size_t verified = 0;
                std::error_code rootError;
                const fs::path canonicalRoot = fs::weakly_canonical(cfg.root, rootError);
                if (rootError)
                    throw std::runtime_error(
                        "cannot resolve project root for package verification");

                if (parsedLock.metadata.at("package") != cfg.name) {
                    std::cerr
                        << "pkg verify: lockfile mismatch: package name differs from nexus.toml\n";
                    failed = true;
                }
                if (parsedLock.metadata.at("version") != cfg.version) {
                    std::cerr << "pkg verify: lockfile mismatch: project version differs from "
                                 "nexus.toml\n";
                    failed = true;
                }

                // Reject stale lockfiles even if every previously locked package
                // still hashes correctly.
                for (const auto& [name, value] : manifestDependencies) {
                    (void)value;
                    if (!lockedDependencies.contains(name)) {
                        std::cerr << "pkg verify: lockfile mismatch: dependency '" << name
                                  << "' is absent from nexus.lock\n";
                        failed = true;
                    }
                }
                for (const auto& [name, version] : lockedDependencies) {
                    (void)version;
                    if (!manifestDependencies.contains(name)) {
                        std::cerr << "pkg verify: lockfile mismatch: stale dependency '" << name
                                  << "' is not declared in nexus.toml\n";
                        failed = true;
                    }
                }

                // Dependencies whose manifest path is inside vendor/ are local
                // packages and require source and integrity records.
                for (const auto& [name, value] : manifestDependencies) {
                    const fs::path declared(value);
                    const fs::path normalized = declared.lexically_normal();
                    if (!declared.is_absolute() && normalized.begin() != normalized.end() &&
                        *normalized.begin() == "vendor") {
                        if (!sources.contains(name)) {
                            std::cerr << "pkg verify: lockfile mismatch: missing source for local "
                                      << "dependency '" << name << "'\n";
                            failed = true;
                        }
                        if (!integrities.contains(name)) {
                            std::cerr
                                << "pkg verify: lockfile mismatch: missing checksum for local "
                                << "dependency '" << name << "'\n";
                            failed = true;
                        }
                        const fs::path localRoot = (cfg.root / normalized).lexically_normal();
                        std::error_code localError;
                        if (!fs::is_regular_file(localRoot / "nexus.toml", localError)) {
                            std::cerr << "pkg verify: missing local package for dependency '"
                                      << name << "'\n";
                            failed = true;
                        }
                    }
                }

                for (const auto& [name, source] : sources) {
                    const auto integrity = integrities.find(name);
                    if (integrity == integrities.end()) {
                        std::cerr << "pkg verify: missing checksum for " << name << "\n";
                        failed = true;
                        continue;
                    }
                    const auto declaredDependency = manifestDependencies.find(name);
                    if (declaredDependency == manifestDependencies.end()) {
                        std::cerr
                            << "pkg verify: lockfile mismatch: source for undeclared dependency '"
                            << name << "'\n";
                        failed = true;
                        continue;
                    }

                    const fs::path relative(source);
                    const fs::path declared(declaredDependency->second);
                    if (relative.is_absolute() || relative.empty() ||
                        relative.begin() == relative.end() || *relative.begin() != "vendor" ||
                        declared.is_absolute() ||
                        declared.lexically_normal().generic_string() !=
                            relative.lexically_normal().generic_string()) {
                        std::cerr << "pkg verify: unsafe or stale source path for " << name << "\n";
                        failed = true;
                        continue;
                    }

                    std::error_code vendorError;
                    const fs::path canonicalVendor =
                        fs::weakly_canonical(canonicalRoot / "vendor", vendorError);
                    if (vendorError) {
                        std::cerr << "pkg verify: cannot resolve vendor directory for " << name
                                  << "\n";
                        failed = true;
                        continue;
                    }
                    const fs::path vendorInsideRoot =
                        canonicalVendor.lexically_relative(canonicalRoot);
                    if (vendorInsideRoot.empty() || *vendorInsideRoot.begin() == "..") {
                        std::cerr << "pkg verify: vendor directory escapes project for " << name
                                  << "\n";
                        failed = true;
                        continue;
                    }

                    const fs::path dependency = (canonicalRoot / relative).lexically_normal();
                    std::error_code dependencyError;
                    const fs::path canonicalDependency =
                        fs::weakly_canonical(dependency, dependencyError);
                    if (dependencyError || !fs::is_directory(canonicalDependency) ||
                        !fs::is_regular_file(canonicalDependency / "nexus.toml")) {
                        std::cerr << "pkg verify: missing local package " << name << "\n";
                        failed = true;
                        continue;
                    }

                    // Project-root containment is insufficient: vendor/../other is
                    // still inside the project. Require the resolved path to remain
                    // strictly below the canonical vendor directory (also rejecting
                    // a vendor symlink that points outside the project).
                    const fs::path insideVendor =
                        canonicalDependency.lexically_relative(canonicalVendor);
                    if (insideVendor.empty() || *insideVendor.begin() == ".." ||
                        *insideVendor.begin() == ".") {
                        std::cerr << "pkg verify: package path escapes vendor for " << name << "\n";
                        failed = true;
                        continue;
                    }

                    try {
                        const auto dependencyConfig = loadProject(canonicalDependency);
                        if (dependencyConfig.name != name) {
                            std::cerr << "pkg verify: package name mismatch for dependency '"
                                      << name << "': package manifest declares '"
                                      << dependencyConfig.name << "'\n";
                            failed = true;
                            continue;
                        }
                        const auto lockedVersion = lockedDependencies.find(name);
                        if (lockedVersion == lockedDependencies.end() ||
                            dependencyConfig.version != lockedVersion->second) {
                            std::cerr << "pkg verify: lockfile mismatch: version changed for "
                                      << name << "\n";
                            failed = true;
                            continue;
                        }
                    } catch (const std::exception& error) {
                        std::cerr << "pkg verify: invalid package '" << name
                                  << "': " << error.what() << "\n";
                        failed = true;
                        continue;
                    }

                    const std::string actual = packageTreeSha256(canonicalDependency);
                    if (actual != integrity->second) {
                        std::cerr << "pkg verify: integrity mismatch for " << name << "\n";
                        failed = true;
                        continue;
                    }
                    ++verified;
                    std::cout << "  verified " << name << " [" << actual << "]\n";
                }
                for (const auto& [name, checksum] : integrities) {
                    (void)checksum;
                    if (!sources.contains(name)) {
                        std::cerr << "pkg verify: checksum has no source entry for " << name
                                  << "\n";
                        failed = true;
                    }
                }
                if (sources.size() != integrities.size()) {
                    std::cerr << "pkg verify: source/checksum records are inconsistent\n";
                    failed = true;
                }
                if (failed)
                    return 1;
                if (sources.empty() && integrities.empty()) {
                    std::cout << "pkg verify: no checksummed local dependencies\n";
                    return 0;
                }
                std::cout << "pkg verify: OK (" << verified << " local package(s))\n";
                return 0;
            }
            if (sub == "build") {
                const auto input = cfg.root / cfg.entry;
                const std::string triple = targetTriple(cfg.target);
                bool windowsTarget = false;
#ifdef _WIN32
                windowsTarget = cfg.target.empty() || cfg.target == "native" ||
                                triple.find("windows") != std::string::npos;
#else
                windowsTarget = triple.find("windows") != std::string::npos;
#endif
                const auto output = defaultOutput(input, "", true, windowsTarget);
                compileFile(input, output, cfg.target, false, 3);
                std::cout << "pkg build: OK -> " << output << "\n";
                return 0;
            }
            if (sub == "publish") {
                if (!validPackageName(cfg.name))
                    throw std::runtime_error("invalid package name in nexus.toml");
                const auto dist = cfg.root / "dist";
                fs::create_directories(dist);
                const auto archive = dist / (cfg.name + "-" + cfg.version + ".nxpkg");
                std::vector<std::string> tarArgv = {
                    "cmake", "-E", "tar", "cf", archive.string(), "--format=zip"};
                for (const auto& relative : {std::string("nexus.toml"),
                                             std::string("src"),
                                             std::string("assets"),
                                             std::string("scenes"),
                                             std::string("vendor"),
                                             std::string("docs")})
                    if (fs::exists(cfg.root / relative))
                        tarArgv.push_back(relative);
                const int rc = runCommand(tarArgv, cfg.root);
                if (rc != 0)
                    throw std::runtime_error("cannot create package archive");
                std::cout << "pkg publish: created local archive " << archive
                          << " (no remote registry upload is configured)\n";
                return 0;
            }
            throw std::runtime_error("unknown pkg command: " + sub);
        }

    } // namespace driver
} // namespace nexus
