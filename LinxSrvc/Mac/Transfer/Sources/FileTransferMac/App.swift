import SwiftUI
import UniformTypeIdentifiers

@main
struct FileTransferApp: App {
    @StateObject private var core = TransferCore()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(core)
                .frame(minWidth: 680, minHeight: 480)
        }
        .windowResizability(.contentMinSize)

        Settings {
            SettingsView()
                .environmentObject(core)
                .frame(width: 400, height: 200)
        }
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
