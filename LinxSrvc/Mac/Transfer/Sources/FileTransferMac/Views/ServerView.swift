// MARK: - Server View (receive files from peer device)
import SwiftUI
import AppKit

struct ServerView: View {
    @EnvironmentObject var core: TransferCore
    @State private var portText = UserDefaults.standard.string(forKey: "defaultPort") ?? "8800"
    @State private var showSettings = false

    var body: some View {
        VStack(spacing: 0) {
            VStack(spacing: 8) {
                HStack {
                    Image(systemName: core.isServerRunning ? "arrow.down.circle.fill" : "arrow.down.circle")
                        .font(.largeTitle)
                        .foregroundColor(core.isServerRunning ? .green : .secondary)
                    Text("Receive Files from Device")
                        .font(.title2.bold())
                    Spacer()

                    Button {
                        showSettings.toggle()
                    } label: {
                        Image(systemName: "gearshape")
                            .font(.title3)
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(.plain)
                    .help("Settings")
                    .accessibilityLabel("Settings")
                    .popover(isPresented: $showSettings, arrowEdge: .bottom) {
                        TransferSettingsPanel()
                    }
                }

                HStack {
                    Text("Your IP:")
                        .foregroundColor(.secondary)
                    Text(core.serverIP)
                        .font(.system(.body, design: .monospaced))
                        .foregroundColor(.blue)
                        .textSelection(.enabled)

                    Text("Port:")
                        .foregroundColor(.secondary)
                        .padding(.leading)

                    if core.isServerRunning {
                        Text(portText)
                            .font(.system(.body, design: .monospaced))
                    } else {
                        MacTextField(text: $portText, placeholder: "8800", width: 90)
                    }

                    Spacer()
                }
            }
            .padding()
            .background(Color(nsColor: .controlBackgroundColor).opacity(0.5))

            Divider()

            HStack {
                Image(systemName: core.isServerRunning
                      ? "antenna.radiowaves.left.and.right"
                      : "antenna.radiowaves.left.and.right.slash")
                    .font(.title3)
                    .foregroundColor(core.isServerRunning ? .green : .secondary)
                Text(shortTransferStatus(core.transferStatus))
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()

                if core.isServerRunning {
                    Button(role: .destructive) {
                        core.stopServer()
                    } label: {
                        Label("STOP", systemImage: "stop.fill")
                    }
                    .buttonStyle(.borderedProminent)
                    .help("Stop listening and disconnect all clients")
                } else {
                    Button {
                        guard let port = UInt16(portText), port > 0 else {
                            core.transferStatus = "Invalid port number"
                            core.appendLog("ERROR: Invalid port: \(portText)")
                            return
                        }
                        core.startServer(port: port)
                    } label: {
                        Label("Start Listening", systemImage: "play.fill")
                    }
                    .buttonStyle(.borderedProminent)
                    .accessibilityLabel("Start server")
                }
            }
            .padding(.horizontal)
            .padding(.vertical, 6)

            Divider()

            if core.isServerRunning {
                if core.totalBytes > 0 {
                    TransferProgressBar(status: core.transferStatus, progress: core.progress,
                                        transferred: core.transferredBytes, total: core.totalBytes,
                                        fileName: core.transferFileName)
                }

                List {
                    Section {
                        if core.devices.isEmpty {
                            HStack {
                                Image(systemName: "info.circle")
                                    .foregroundColor(.secondary)
                                Text("Waiting at \(core.serverIP):\(portText)")
                                    .foregroundColor(.secondary)
                            }
                        } else {
                            ForEach(core.devices) { device in
                                VStack(alignment: .leading, spacing: 2) {
                                    HStack(spacing: 8) {
                                        Circle()
                                            .fill(device.online
                                                  ? Color.green
                                                  : Color.secondary.opacity(0.5))
                                            .frame(width: 8, height: 8)
                                        Text("\(device.ip):\(device.port)")
                                            .font(.system(.body, design: .monospaced))
                                        Spacer()
                                        Text(device.online ? "Connected"
                                             : "Last seen " + timeFormatter.string(from: device.lastSeen))
                                            .font(.caption)
                                            .foregroundColor(.secondary)
                                    }
                                    // Live receive state for this client; empty name means idle.
                                    if !device.transferringFile.isEmpty {
                                        HStack(spacing: 6) {
                                            Image(systemName: "arrow.down.doc")
                                                .font(.caption2)
                                                .foregroundColor(.blue)
                                            Text(device.transferringFile)
                                                .font(.caption)
                                                .lineLimit(1)
                                                .truncationMode(.middle)
                                            Spacer()
                                            if device.total > 0 {
                                                let pct = Int(Double(device.transferred)
                                                              / Double(device.total) * 100)
                                                Text("\(formatBytes(device.transferred)) / "
                                                     + "\(formatBytes(device.total)) (\(pct)%)")
                                                    .font(.caption.monospacedDigit())
                                                    .foregroundColor(.secondary)
                                            }
                                        }
                                        .padding(.leading, 16)
                                    }
                                }
                            }
                        }
                    } header: {
                        // Connection count summary: online sessions vs. every known device.
                        let onlineCount = core.devices.filter(\.online).count
                        Text("Devices — \(onlineCount) connected, \(core.devices.count) total")
                    }

                    if !core.receivedFiles.isEmpty {
                        Section("Received Files — \(core.receivedFiles.count)") {
                            ForEach(core.receivedFiles) { file in
                                ReceivedFileRow(file: file)
                            }
                        }
                    }
                }
                .listStyle(.inset)
            } else {
                VStack(spacing: 20) {
                    Image(systemName: "arrow.down.doc")
                        .font(.system(size: 48))
                        .foregroundColor(.secondary)
                    Text("Start listening to receive files.")
                        .foregroundColor(.secondary)
                }
                .frame(maxHeight: .infinity)
            }
        }
        // Connection events refresh devices immediately; this low-frequency reconcile guards against missed events.
        .task(id: core.isServerRunning) {
            while core.isServerRunning && !Task.isCancelled {
                core.refreshDevices()
                try? await Task.sleep(nanoseconds: 2_000_000_000)
            }
        }
        .onChange(of: core.defaultPort) { newValue in
            // Settings changes take effect immediately; never override a running server's port.
            if !core.isServerRunning { portText = newValue }
        }
    }
}

// MARK: - Settings panel (shown from the gear button in the Server tab)

struct TransferSettingsPanel: View {
    @EnvironmentObject var core: TransferCore

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("Settings")
                .font(.headline)

            Toggle("Exponential backoff", isOn: $core.backoffEnabled)
            Text("Retry a connection that died before any data arrived (ENOTCONN). "
                + "On: up to 16 retries, delay doubles from 10ms (~655s worst case). "
                + "Off (default): 3 short retries, then close.")
                .font(.caption)
                .foregroundColor(.secondary)
                .fixedSize(horizontal: false, vertical: true)

            Divider()

            Text("Receive Location")
                .font(.subheadline.bold())
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
        .padding()
        .frame(width: 340)
    }
}
