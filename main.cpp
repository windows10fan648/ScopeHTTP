// Crow installs an implicit /static/<path> route by default. ScopeHTTP owns
// that URL and performs its own document-root and symlink validation, so move
// Crow's unused built-in route out of the public URL space.
#define CROW_STATIC_DIRECTORY "__scopehttp_builtin_static_disabled__/"
#define CROW_STATIC_ENDPOINT "/__scopehttp_builtin_static_disabled/<path>"
#include <crow.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

constexpr unsigned short kPort = 8080;

// Resolve the document root beside the executable. SCOPEHTTP_PUBLIC_DIR is
// supported for deployments that keep assets in a separate location.
fs::path public_directory(int argc, char* argv[])
{
    if (const char* configured = std::getenv("SCOPEHTTP_PUBLIC_DIR");
        configured != nullptr && *configured != '\0') {
        return fs::weakly_canonical(fs::path(configured));
    }

    const fs::path executable = argc > 0 ? fs::absolute(argv[0]) : fs::current_path();
    return fs::weakly_canonical(executable.parent_path() / "public");
}

bool is_inside(const fs::path& root, const fs::path& candidate)
{
    const fs::path relative = candidate.lexically_relative(root);
    if (relative.empty() || relative.is_absolute()) {
        return false;
    }

    for (const fs::path& component : relative) {
        if (component == "..") {
            return false;
        }
    }

    return true;
}

crow::response file_response(const fs::path& public_root, const std::string& url_path)
{
    // Crow decodes the route parameter before invoking the handler. Reject
    // absolute paths and traversal components before filesystem resolution.
    const fs::path requested(url_path);
    if (requested.is_absolute()) {
        return crow::response(400, "Invalid file path");
    }

    std::error_code error;
    const fs::path resolved = fs::weakly_canonical(public_root / requested, error);
    if (error || !is_inside(public_root, resolved) || !fs::is_regular_file(resolved, error)) {
        return crow::response(404, "Not found");
    }

    crow::response response;
    // The path has already been canonicalized and constrained to public_root,
    // so bypass Crow's URL-oriented sanitizer. On Windows that sanitizer can
    // misinterpret an absolute drive-qualified path such as C:\\... as a URL
    // path and produce a 404.
    response.set_static_file_info_unsafe(resolved.string());
    return response;
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        const fs::path public_root = public_directory(argc, argv);
        std::error_code error;
        const fs::path index_file = fs::weakly_canonical(public_root / "index.html", error);

        if (error || !is_inside(public_root, index_file) ||
            !fs::is_regular_file(index_file, error)) {
            std::cerr << "ScopeHTTP: missing required file: "
                      << (public_root / "index.html") << '\n';
            return EXIT_FAILURE;
        }

        crow::SimpleApp app;
        app.server_name("ScopeHTTP/1.0")
           .bindaddr("127.0.0.1")
           .port(kPort)
           .concurrency(2)
           .loglevel(crow::LogLevel::Info);

        CROW_ROUTE(app, "/")([index_file](crow::response& response) {
            response.set_static_file_info_unsafe(index_file.string());
            response.end();
        });

        // Files are available at /static/<path>, for example:
        // /static/site.css or /static/images/logo.png.
        CROW_ROUTE(app, "/static/<path>")
        ([public_root](const std::string& path) {
            return file_response(public_root, path);
        });

        CROW_ROUTE(app, "/api/status")([] {
            crow::json::wvalue status;
            status["service"] = "ScopeHTTP";
            status["status"] = "ok";
            status["version"] = "1.0.0";
            return status;
        });

        std::cout << "ScopeHTTP listening on http://127.0.0.1:" << kPort << '\n'
                  << "Serving static files from " << public_root << '\n';
        app.run();
    } catch (const std::exception& exception) {
        std::cerr << "ScopeHTTP fatal error: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
