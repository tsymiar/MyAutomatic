// MARK: - Client View (send files to peer device)
import SwiftUI
import AppKit

struct ClientView: View {
    @EnvironmentObject var core: TransferCore
    @State private var isDragging = false
    @State private var pullFileName = ""

    var body: some View {
        VStack(spacing: 0) {
            header
            Divider()
            statusBar
            Divider()
            content
        }
        // Claim full tab height, top-aligned: without a greedy child (no tasks
        // yet) the view would center vertically, leaving a blank band above.
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
    }

    // MARK: - Header

    private var header: some View {
        VStack(spacing: 8) {
            HStack {
                Image(systemName: core.isConnected ? "arrow.up.circle.fill" : "arrow.up.circle")
                    .font(.largeTitle)
                    .foregroundColor(core.isConnected ? .orange : .secondary)
                Text("Send Files to Device")
                    .font(.title2.bold())
                Spacer()
            }

            HStack {
                Text("Target IP:")
                    .foregroundColor(.secondary)
                MacTextField(text: $core.targetIP, placeholder: "192.168.x.x",
                             width: 180, isDisabled: core.isConnected)

                Text("Port:")
                    .foregroundColor(.secondary)
                MacTextField(text: $core.targetPort, placeholder: "8800",
                             width: 90, isDisabled: core.isConnected)

                Spacer()

                connectButton
            }
        }
        .padding()
        .background(Color(nsColor: .controlBackgroundColor).opacity(0.5))
    }

    @ViewBuilder
    private var connectButton: some View {
        if core.isConnected {
            Button(role: .destructive) {
                core.disconnect()
            } label: {
                Label("Disconnect", systemImage: "link.slash")
            }
            .buttonStyle(.borderedProminent)
            // Disable disconnect while sending: destroying the engine while a
            // background call is blocked in ft_send_file is use-after-free.
            .disabled(core.isBusy)
        } else {
            Button {
                connect()
            } label: {
                Label("Connect", systemImage: "link")
            }
            .buttonStyle(.borderedProminent)
            .disabled(core.targetIP.isEmpty)
        }
    }

    // MARK: - Status bar

    private var statusBar: some View {
        HStack {
            Image(systemName: core.isConnected ? "checkmark.circle.fill" : "xmark.circle")
                .foregroundColor(core.isConnected ? .green : .secondary)
            Text(shortTransferStatus(core.transferStatus))
                .font(.caption)
                .foregroundColor(.secondary)
            Spacer()
        }
        .padding(.horizontal)
        .padding(.vertical, 6)
    }

    // MARK: - Content

    @ViewBuilder
    private var content: some View {
        if core.isConnected {
            connectedContent
        } else {
            idleContent
        }
    }

    private var connectedContent: some View {
        VStack {
            // Only for transfers started from this tab: totalBytes is shared
            // core state, so a server-side receive must not inflate this view.
            if core.isBusy || core.pendingDeleteFile != nil {
                HStack {
                    TransferProgressBar(status: core.transferStatus, progress: core.progress,
                                        transferred: core.transferredBytes, total: core.totalBytes,
                                        compact: true, fileName: core.transferFileName)
                    if core.isBusy {
                        Button {
                            core.cancelTransfer()
                        } label: {
                            Image(systemName: "xmark.circle.fill")
                                .font(.title3)
                                .foregroundColor(.red)
                        }
                        .buttonStyle(.plain)
                        .padding(.trailing)
                        .help("Cancel the ongoing transfer")
                    } else if core.pendingDeleteFile != nil {
                        // Cancelled: the peer keeps the partial file — offer deletion
                        Button {
                            core.deletePartialFile()
                        } label: {
                            Image(systemName: "trash.fill")
                                .font(.title3)
                                .foregroundColor(.red)
                        }
                        .buttonStyle(.plain)
                        .padding(.trailing)
                        .help("Delete the partial file left on the peer")
                    }
                }
            }

            dropZone

            pullSection

            if !core.transferTasks.isEmpty {
                List {
                    Section("Transfer Tasks") {
                        ForEach(core.transferTasks) { task in
                            TransferRow(task: task)
                        }
                    }
                }
                .listStyle(.inset)
            }
        }
    }

    private var dropZone: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 12)
                .strokeBorder(style: StrokeStyle(lineWidth: 2, dash: [8, 4]))
                .foregroundColor(isDragging ? .accentColor : .secondary.opacity(0.4))
                .background(
                    RoundedRectangle(cornerRadius: 12)
                        .fill(isDragging ? Color.accentColor.opacity(0.08) : Color.clear)
                )

            VStack(spacing: 12) {
                Image(systemName: "doc.badge.plus")
                    .font(.system(size: 36))
                    .foregroundColor(.secondary)
                Text("Drop files here")
                    .font(.headline)
                    .foregroundColor(.secondary)
                Text("or")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Button {
                    pickAndSendFiles()
                } label: {
                    Label("Select Files…", systemImage: "folder")
                }
                .disabled(core.isBusy)
            }
        }
        .frame(height: 160)
        .padding()
        .onDrop(of: [.fileURL], isTargeted: $isDragging) { providers in
            handleDrop(providers)
            return true
        }
    }

    // MARK: - Pull from peer

    private var pullSection: some View {
        HStack {
            Image(systemName: "arrow.down.doc")
                .foregroundColor(.secondary)
            Text("Remote file:")
                .foregroundColor(.secondary)
            MacTextField(text: $pullFileName, placeholder: "e.g. report.pdf",
                         width: 220, isDisabled: core.isBusy)
            Button {
                core.requestFile(pullFileName)
            } label: {
                Label("Pull from Peer", systemImage: "arrow.down.to.line")
            }
            // Mutually exclusive with sending: push/pull must not interleave
            // on a single connection
            .disabled(core.isBusy
                      || pullFileName.trimmingCharacters(in: .whitespaces).isEmpty)
            Spacer()
        }
        .padding(.horizontal)
    }

    private var idleContent: some View {
        VStack(spacing: 20) {
            Image(systemName: "arrow.up.doc")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            Text("Connect to a device running the transfer server,\nthen drag files here or click to select.")
                .multilineTextAlignment(.center)
                .foregroundColor(.secondary)
        }
        .frame(maxHeight: .infinity)
    }

    // MARK: - Actions

    private func connect() {
        guard let port = UInt16(core.targetPort), port > 0 else {
            core.transferStatus = "Invalid port number: \(core.targetPort)"
            core.appendLog("ERROR: Invalid port: \(core.targetPort)")
            return
        }
        core.connect(to: core.targetIP, port: port)
    }

    private func handleDrop(_ providers: [NSItemProvider]) {
        for provider in providers {
            _ = provider.loadObject(ofClass: URL.self) { url, _ in
                guard let url else { return }
                DispatchQueue.main.async {
                    core.postLocalFile(url.path)
                }
            }
        }
    }

    private func pickAndSendFiles() {
        let panel = NSOpenPanel()
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = true
        if panel.runModal() == .OK {
            for url in panel.urls {
                core.postLocalFile(url.path)
            }
        }
    }
}
