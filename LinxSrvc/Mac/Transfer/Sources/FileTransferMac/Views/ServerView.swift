// MARK: - Server View (receive files from peer device)
import SwiftUI
import AppKit

struct ServerView: View {
    @EnvironmentObject var core: TransferCore
    @State private var portText = "8800"
    @State private var pollTimer: Timer?

    var body: some View {
        VStack(spacing: 0) {
            // Header
            VStack(spacing: 8) {
                HStack {
                    Image(systemName: core.isServerRunning ? "arrow.down.circle.fill" : "arrow.down.circle")
                        .font(.largeTitle)
                        .foregroundColor(core.isServerRunning ? .green : .secondary)
                    Text("Receive Files from Device")
                        .font(.title2.bold())
                    Spacer()
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

                    if core.isServerRunning {
                        HStack(spacing: 4) {
                            Circle()
                                .fill(.green)
                                .frame(width: 8, height: 8)
                            Text("\(core.clientCount) client(s)")
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }
                    }
                }
            }
            .padding()
            .background(Color(nsColor: .controlBackgroundColor).opacity(0.5))

            Divider()

            // Status bar
            HStack {
                Image(systemName: core.isServerRunning
                      ? "antenna.radiowaves.left.and.right"
                      : "antenna.radiowaves.left.and.right.slash")
                    .foregroundColor(core.isServerRunning ? .green : .secondary)
                Text(core.transferStatus)
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()

                if core.isServerRunning {
                    Button(role: .destructive) {
                        core.stopServer()
                        pollTimer?.invalidate()
                    } label: {
                        Label("STOP", systemImage: "stop.fill")
                    }
                    .buttonStyle(.borderedProminent)
                } else {
                    Button {
                        guard let port = UInt16(portText), port > 0 else {
                            core.transferStatus = "Invalid port number"
                            core.appendLog("ERROR: Invalid port: \(portText)")
                            return
                        }
                        core.startServer(port: port)
                        pollTimer = Timer.scheduledTimer(withTimeInterval: 1.5, repeats: true) { _ in
                            MainActor.assumeIsolated {
                                core.updateClientCount()
                            }
                        }
                    } label: {
                        Label("Start Listening", systemImage: "play.fill")
                    }
                    .buttonStyle(.borderedProminent)
                }
            }
            .padding(.horizontal)
            .padding(.vertical, 6)

            Divider()

            // Content
            if core.isServerRunning {
                if core.totalBytes > 0 {
                    TransferProgressBar(status: core.transferStatus, progress: core.progress,
                                        transferred: core.transferredBytes, total: core.totalBytes)
                }

                List {
                    Section("Connected Devices") {
                        if core.clientCount == 0 {
                            HStack {
                                Image(systemName: "info.circle")
                                    .foregroundColor(.secondary)
                                Text("Waiting for device to connect to \(core.serverIP):\(portText)")
                                    .foregroundColor(.secondary)
                            }
                        } else {
                            HStack {
                                Image(systemName: "iphone.gen2")
                                Text("\(core.clientCount) device(s) connected")
                                Spacer()
                                Text("●")
                                    .foregroundColor(.green)
                                    .font(.caption)
                            }
                        }
                    }

                    if !core.receivedFiles.isEmpty {
                        Section("Received Files") {
                            ForEach(core.receivedFiles) { file in
                                HStack {
                                    Image(systemName: "doc.fill")
                                        .foregroundColor(.blue)
                                    VStack(alignment: .leading) {
                                        Text(file.fileName)
                                            .lineLimit(1)
                                        Text("\(formatBytes(file.fileSize)) • from \(file.fromIP)")
                                            .font(.caption)
                                            .foregroundColor(.secondary)
                                    }
                                    Spacer()
                                    Button {
                                        revealInFinder(file)
                                    } label: {
                                        Image(systemName: "folder.fill")
                                            .foregroundColor(.accentColor)
                                    }
                                    .buttonStyle(.plain)
                                    .help("Show in Finder")
                                }
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
                    Text("Start the server to receive files from another device.\n"
                         + "The peer device should connect to the IP and port shown above.")
                        .multilineTextAlignment(.center)
                        .foregroundColor(.secondary)
                }
                .frame(maxHeight: .infinity)
            }
        }
        .onDisappear {
            pollTimer?.invalidate()
        }
    }

    private func revealInFinder(_ file: ReceivedFile) {
        if FileManager.default.fileExists(atPath: file.savedPath.path) {
            NSWorkspace.shared.activateFileViewerSelecting([file.savedPath])
        } else {
            NSWorkspace.shared.selectFile(nil, inFileViewerRootedAtPath: core.savePath)
        }
    }
}
