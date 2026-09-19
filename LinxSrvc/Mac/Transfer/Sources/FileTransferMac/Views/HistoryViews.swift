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
                        core.transferTasks.removeAll()
                        core.receivedFiles.removeAll()
                    }
                    .buttonStyle(.borderless)
                    .font(.caption)
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
                                HStack {
                                    Image(systemName: "doc.fill")
                                        .foregroundColor(.blue)
                                    VStack(alignment: .leading) {
                                        Text(file.fileName).lineLimit(1)
                                        Text("\(formatBytes(file.fileSize)) • \(file.fromIP) • "
                                             + "\(timeFormatter.string(from: file.receivedAt))")
                                            .font(.caption)
                                            .foregroundColor(.secondary)
                                    }
                                }
                            }
                        }
                    }
                }
                .listStyle(.inset)
            }
        }
    }
}

// MARK: - Shared Views

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
                    Text("•")
                    Text(task.direction == .sending ? "→" : "←")
                    Text(task.peerIP)
                }
                .font(.caption)
                .foregroundColor(.secondary)
            }
            Spacer()
            if task.status == .transferring {
                Text("\(Int(task.progress * 100))%")
                    .font(.caption.monospacedDigit())
                    .foregroundColor(.accentColor)
            } else if task.status == .completed {
                Text("Done")
                    .font(.caption)
                    .foregroundColor(.green)
            } else if task.status == .failed {
                Text("Failed")
                    .font(.caption)
                    .foregroundColor(.red)
            }
        }
        .padding(.vertical, 2)
    }
}

// MARK: - Console Log View

struct LogConsole: View {
    let messages: [String]
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
                Text("\(messages.count) entries")
                    .font(.caption2)
                    .foregroundColor(.secondary)

                Button {
                    copyAll()
                } label: {
                    Image(systemName: "doc.on.doc")
                        .font(.caption)
                }
                .buttonStyle(.borderless)
                .help("Copy all")

                Button {
                    onClear?()
                } label: {
                    Image(systemName: "trash")
                        .font(.caption)
                }
                .buttonStyle(.borderless)
                .help("Clear logs")
            }
            .padding(.horizontal)
            .padding(.vertical, 4)

            ScrollViewReader { proxy in
                ScrollView(.vertical) {
                    LazyVStack(alignment: .leading, spacing: 2) {
                        ForEach(Array(messages.enumerated()), id: \.offset) { idx, msg in
                            HStack(spacing: 4) {
                                Circle()
                                    .fill(logColor(msg))
                                    .frame(width: 5, height: 5)
                                Text(msg)
                                    .font(.system(size: 10, design: .monospaced))
                                    .lineLimit(2)
                                    .foregroundColor(logColor(msg).opacity(0.8))
                                    .textSelection(.enabled)
                                Spacer(minLength: 0)
                            }
                            .padding(.horizontal, 10)
                            .padding(.vertical, 1)
                            .id(idx)
                        }
                    }
                }
                .onChange(of: messages.count) { _ in
                    if let lastIdx = messages.indices.last {
                        withAnimation {
                            proxy.scrollTo(lastIdx, anchor: .bottom)
                        }
                    }
                }
            }
        }
    }

    private func copyAll() {
        let all = messages.joined(separator: "\n")
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
