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

// MARK: - Settings

struct SettingsView: View {
    @EnvironmentObject var core: TransferCore

    var body: some View {
        Form {
            Section("Receive Location") {
                HStack {
                    Text(core.savePath)
                        .lineLimit(1)
                        .truncationMode(.middle)
                        .foregroundColor(.secondary)
                    Spacer()
                    Button("Choose…") {
                        let panel = NSOpenPanel()
                        panel.canChooseDirectories = true
                        panel.canChooseFiles = false
                        panel.canCreateDirectories = true
                        panel.prompt = "Select"
                        if panel.runModal() == .OK, let url = panel.url {
                            core.savePath = url.path
                        }
                    }
                }
            }
        }
        .padding()
        .formStyle(.grouped)
    }
}
