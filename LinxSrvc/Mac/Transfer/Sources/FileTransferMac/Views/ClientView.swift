// MARK: - Client View (send files to peer device)
import SwiftUI
import AppKit

struct ClientView: View {
    @EnvironmentObject var core: TransferCore
    @State private var isDragging = false

    var body: some View {
        VStack(spacing: 0) {
            header
            Divider()
            statusBar
            Divider()
            content
        }
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
            Text(core.transferStatus)
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
            if core.totalBytes > 0 || core.isBusy {
                TransferProgressBar(status: core.transferStatus, progress: core.progress,
                                    transferred: core.transferredBytes, total: core.totalBytes)
            }

            dropZone

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
