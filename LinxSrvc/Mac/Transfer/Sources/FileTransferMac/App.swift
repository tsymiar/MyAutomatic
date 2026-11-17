import SwiftUI
import AppKit
import UniformTypeIdentifiers

@main
struct FileTransferApp: App {
    @StateObject private var core = TransferCore()

    init() {
        // Keep the window tab bar available.
        NSWindow.allowsAutomaticWindowTabbing = true
    }

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(core)
                .frame(minWidth: 680, minHeight: 480)
                .background(TabTitle())
        }
        .windowResizability(.contentMinSize)

        Settings {
            SettingsView()
                .environmentObject(core)
                .frame(width: 400, height: 200)
        }
    }
}

/// Number each window tab from its position in the tab group.
private struct TabTitle: NSViewRepresentable {
    func makeNSView(context: Context) -> NSView {
        let view = NSView()
        apply(to: view)
        DispatchQueue.main.async { self.apply(to: view) }
        return view
    }

    func updateNSView(_ view: NSView, context: Context) {
        apply(to: view)
    }

    private func apply(to view: NSView) {
        guard let window = view.window else { return }
        window.tabbingMode = .preferred
        if let tabGroup = window.tabGroup, !tabGroup.isTabBarVisible {
            window.toggleTabBar(nil)
        }
        let tabs = window.tabbedWindows ?? [window]
        let index = tabs.firstIndex(where: { $0 === window }) ?? 0
        window.tab.title = "T\(index + 1)"
    }
}

// MARK: - Settings (⌘,) — miscellaneous options not covered by the Server-tab gear

struct SettingsView: View {
    @EnvironmentObject var core: TransferCore

    var body: some View {
        Form {
            HStack {
                Text("Default port")
                Spacer()
                MacTextField(text: $core.defaultPort, placeholder: "8800", width: 90)
            }
            Text("Pre-fills the Server and Client port fields.")
                .font(.caption)
                .foregroundColor(.secondary)
                .fixedSize(horizontal: false, vertical: true)

            Toggle("Show log console", isOn: $core.showLogConsole)
        }
        .padding()
        .formStyle(.grouped)
    }
}
