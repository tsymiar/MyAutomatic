// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "FileTransfer",
    platforms: [.macOS(.v13)],
    targets: [
        // ── C++ core library ──────────────────────────
        .target(
            name: "FileTransferCore",
            path: "Sources/FileTransferCore",
            sources: [
                "TransferEngine.cpp",
                "TransferBridge.cpp"
            ],
            publicHeadersPath: "include",
            cxxSettings: [
                .define("LOG_TAG", to: "\"FileTransfer\""),
                .unsafeFlags(["-std=c++17"])
            ],
            linkerSettings: [
                .linkedLibrary("c++")
            ]
        ),
        // ── Swift UI application ──────────────────────
        .executableTarget(
            name: "FileTransferMac",
            dependencies: ["FileTransferCore"],
            path: "Sources/FileTransferMac"
        )
    ]
)
