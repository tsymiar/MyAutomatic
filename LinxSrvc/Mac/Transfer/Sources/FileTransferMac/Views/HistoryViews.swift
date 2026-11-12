// MARK: - Transfer Log View
import SwiftUI
import AppKit

struct TransferLogView: View {
    @EnvironmentObject var core: TransferCore

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Image(systemName: "clock.arrow.circlepath")
                    .font(.largeTitle)
                    .foregroundColor(.secondary)
                Text("Transfer History")
                    .font(.title2.bold())
                Spacer()

                if !core.transferTasks.isEmpty || !core.receivedFiles.isEmpty {
                    Button("Clear") {
                        // Keep in-flight rows so the completion callback can still find its task by id.
                        core.transferTasks.removeAll { task in
                            !core.inFlightTaskIDs.contains(task.id)
                        }
                        core.inFlightTaskIDs.removeAll { id in
                            !core.transferTasks.contains { $0.id == id }
                        }
                        core.receivedFiles.removeAll()
                    }
                    .buttonStyle(.borderless)
                    .font(.caption)
                    .disabled(core.isBusy)
                    .help("Clear finished transfer history")
                    .accessibilityLabel("Clear transfer history")
                }
            }
            .padding()
            .background(Color(nsColor: .controlBackgroundColor).opacity(0.5))

            Divider()

            if core.transferTasks.isEmpty && core.receivedFiles.isEmpty {
                VStack(spacing: 16) {
                    Image(systemName: "tray")
                        .font(.system(size: 40))
                        .foregroundColor(.secondary)
                    Text("No transfer history yet")
                        .foregroundColor(.secondary)
                }
                .frame(maxHeight: .infinity)
            } else {
                List {
                    if !core.transferTasks.isEmpty {
                        Section("Tasks") {
                            ForEach(core.transferTasks) { task in
                                TransferRow(task: task)
                            }
                        }
                    }
                    if !core.receivedFiles.isEmpty {
                        Section("Received") {
                            ForEach(core.receivedFiles) { file in
                                ReceivedFileRow(file: file)
                            }
                        }
                    }
                }
                .listStyle(.inset)
            }
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

// MARK: - Shared Views

struct ReceivedFileRow: View {
    @EnvironmentObject var core: TransferCore
    let file: ReceivedFile

    var body: some View {
        HStack {
            Image(systemName: "doc.fill")
                .foregroundColor(.blue)
            VStack(alignment: .leading) {
                Text(file.fileName).lineLimit(1)
                Text("\(formatBytes(file.fileSize)) • "
                     + "\(timeFormatter.string(from: file.receivedAt))")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            Spacer()
            Button {
                revealInFinder()
            } label: {
                Image(systemName: "folder.fill")
                    .foregroundColor(.accentColor)
            }
            .buttonStyle(.plain)
            .help("Show in Finder")
            .accessibilityLabel("Show in Finder")
        }
    }

    private func revealInFinder() {
        if FileManager.default.fileExists(atPath: file.savedPath.path) {
            NSWorkspace.shared.activateFileViewerSelecting([file.savedPath])
        } else {
            NSWorkspace.shared.selectFile(nil, inFileViewerRootedAtPath: core.savePath)
        }
    }
}

struct TransferRow: View {
    let task: TransferTask

    var icon: some View {
        Group {
            switch task.status {
            case .pending:     Image(systemName: "clock").foregroundColor(.secondary)
            case .transferring: ProgressView().scaleEffect(0.6)
            case .completed:   Image(systemName: "checkmark.circle.fill").foregroundColor(.green)
            case .failed:      Image(systemName: "xmark.circle.fill").foregroundColor(.red)
            case .cancelled:   Image(systemName: "slash.circle.fill").foregroundColor(.orange)
            }
        }
    }

    var body: some View {
        HStack(spacing: 8) {
            icon
                .frame(width: 20)
            VStack(alignment: .leading, spacing: 2) {
                Text(task.fileName)
                    .lineLimit(1)
                HStack(spacing: 8) {
                    Text(formatBytes(task.fileSize))
                    Text(task.direction == .sending ? "→" : "←")
                    if !task.peerIP.isEmpty { Text(task.peerIP) }
                }
                .font(.caption)
                .foregroundColor(.secondary)
            }
            Spacer()
            if task.status == .transferring {
                VStack(alignment: .trailing, spacing: 2) {
                    Text("\(Int(task.progress * 100))%")
                        .font(.caption.monospacedDigit())
                        .foregroundColor(.accentColor)
                    ProgressView(value: task.progress)
                        .progressViewStyle(.linear)
                        .frame(width: 120)
                }
            }
        }
        .padding(.vertical, 2)
    }
}

// MARK: - Console Log View

struct LogConsole: View {
    let messages: [LogEntry]
    var onClear: (() -> Void)?

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Image(systemName: "text.alignleft")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Text("Console")
                    .font(.caption.bold())
                    .foregroundColor(.secondary)
                Spacer()

                Button {
                    copyAll()
                } label: {
                    Image(systemName: "doc.on.doc")
                        .font(.caption)
                }
                .buttonStyle(.borderless)
                .help("Copy all")
                .accessibilityLabel("Copy all logs")

                Button {
                    onClear?()
                } label: {
                    Image(systemName: "trash")
                        .font(.caption)
                }
                .buttonStyle(.borderless)
                .help("Clear logs")
                .accessibilityLabel("Clear logs")
            }
            .padding(.horizontal)
            .padding(.vertical, 4)

            ScrollViewReader { proxy in
                ScrollView(.vertical) {
                    LazyVStack(alignment: .leading, spacing: 2) {
                        ForEach(messages) { entry in
                            HStack(spacing: 4) {
                                Circle()
                                    .fill(logColor(entry.text))
                                    .frame(width: 5, height: 5)
                                Text(entry.text)
                                    .font(.system(size: 10, design: .monospaced))
                                    .lineLimit(2)
                                    .foregroundColor(logColor(entry.text).opacity(0.8))
                                    .textSelection(.enabled)
                                Spacer(minLength: 0)
                            }
                            .padding(.horizontal, 10)
                            .padding(.vertical, 1)
                            .id(entry.id)
                        }
                    }
                }
                .onChange(of: messages.last?.id) { _ in
                    if let last = messages.last {
                        proxy.scrollTo(last.id, anchor: .bottom)
                    }
                }
            }
            .overlay {
                if messages.isEmpty {
                    Text("No log entries")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
        }
    }

    private func copyAll() {
        let all = messages.map(\.text).joined(separator: "\n")
        NSPasteboard.general.clearContents()
        NSPasteboard.general.setString(all, forType: .string)
    }

    private func logColor(_ msg: String) -> Color {
        if msg.contains("[ERROR]") || msg.contains("ERROR") { return .red }
        if msg.contains("[WARN]")  || msg.contains("fail") || msg.contains("cancel") { return .orange }
        if msg.contains("complete") || msg.contains("connected") || msg.contains("started") { return .green }
        if msg.contains("[DEBUG]") { return .purple }
        return .secondary
    }
}
