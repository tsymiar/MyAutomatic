import SwiftUI
import AppKit
import UniformTypeIdentifiers

// ──────────────────────────────────────────────────────────────────────────────
//  Native NSTextField wrapper — bypasses SwiftUI responder-chain issues on macOS
// ──────────────────────────────────────────────────────────────────────────────

struct MacTextField: NSViewRepresentable {
    @Binding var text: String
    let placeholder: String
    var width: CGFloat?
    var isDisabled: Bool = false

    func makeNSView(context: Context) -> NSTextField {
        let field = NSTextField()
        field.placeholderString = placeholder
        field.isEditable = !isDisabled
        field.isSelectable = !isDisabled
        field.isBordered = true
        field.bezelStyle = .roundedBezel
        field.focusRingType = .default
        field.font = NSFont.systemFont(ofSize: NSFont.systemFontSize)
        field.delegate = context.coordinator
        field.translatesAutoresizingMaskIntoConstraints = false
        if let w = width {
            field.widthAnchor.constraint(equalToConstant: w).isActive = true
        }
        return field
    }

    func updateNSView(_ nsView: NSTextField, context: Context) {
        if nsView.stringValue != text {
            nsView.stringValue = text
        }
        nsView.isEditable = !isDisabled
        nsView.isSelectable = !isDisabled
    }

    func makeCoordinator() -> Coordinator {
        Coordinator(text: $text)
    }

    final class Coordinator: NSObject, NSTextFieldDelegate {
        private var text: Binding<String>

        init(text: Binding<String>) {
            self.text = text
        }

        func controlTextDidChange(_ obj: Notification) {
            guard let field = obj.object as? NSTextField else { return }
            text.wrappedValue = field.stringValue
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Shared: progress bar used by ServerView & ClientView
// ──────────────────────────────────────────────────────────────────────────────

struct TransferProgressBar: View {
    let status: String
    let progress: Double
    let transferred: UInt64
    let total: UInt64

    var body: some View {
        ProgressView(value: progress) {
            HStack {
                Text(status).font(.caption)
                Spacer()
                Text("\(formatBytes(transferred)) / \(formatBytes(total))")
                    .font(.caption.monospacedDigit())
                    .foregroundColor(.secondary)
            }
        }
        .progressViewStyle(.linear)
        .padding()
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  ContentView — 3-tab file transfer UI backed by TransferCore (C++ core)
// ──────────────────────────────────────────────────────────────────────────────

struct ContentView: View {
    @EnvironmentObject var core: TransferCore
    @State private var selectedTab = 0

    var body: some View {
        VStack(spacing: 0) {
            // Use Picker + conditional content instead of TabView.
            // macOS NSTabView (backing SwiftUI TabView) sets
            // refusesFirstResponder=YES on its content, which breaks
            // all TextField / NSTextField focus.
            //
            // All text fields use MacTextField (native NSTextField via
            // NSViewRepresentable), which bypasses SwiftUI responder-chain
            // issues entirely.
            Picker("Mode", selection: $selectedTab) {
                Label("Receive", systemImage: "arrow.down.circle").tag(0)
                Label("Send",    systemImage: "arrow.up.circle").tag(1)
                Label("History", systemImage: "clock").tag(2)
            }
            .pickerStyle(.segmented)
            .padding(.horizontal)
            .padding(.top, 8)

            Divider()
                .padding(.top, 8)

            // Client-side text field state lives in TransferCore so
            // values persist across tab switches even when the view
            // is recreated via switch.
            Group {
                switch selectedTab {
                case 0: ServerView()
                case 1: ClientView()
                case 2: TransferLogView()
                default: EmptyView()
                }
            }

            if !core.logMessages.isEmpty {
                Divider()
                LogConsole(messages: core.logMessages)
                    .frame(height: 140)
            }
        }
    }
}

// MARK: - Server View (receive files from peer device)

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
                Image(systemName: core.isServerRunning ? "antenna.radiowaves.left.and.right" : "antenna.radiowaves.left.and.right.slash")
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
                        Label("Stop", systemImage: "stop.fill")
                    }
                    .buttonStyle(.borderedProminent)
                } else {
                    Button {
                        core.startServer(port: UInt16(portText) ?? 8800)
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
                    Text("Start the server to receive files from another device.\nThe peer device should connect to the IP and port shown above.")
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

// MARK: - Client View (send files to peer device)

struct ClientView: View {
    @EnvironmentObject var core: TransferCore
    @State private var isDragging = false

    var body: some View {
        VStack(spacing: 0) {
            // Header
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
                    MacTextField(text: $core.targetIP, placeholder: "192.168.x.x", width: 180, isDisabled: core.isConnected)

                    Text("Port:")
                        .foregroundColor(.secondary)
                    MacTextField(text: $core.targetPort, placeholder: "8800", width: 90, isDisabled: core.isConnected)

                    Spacer()

                    if core.isConnected {
                        Button(role: .destructive) {
                            core.disconnect()
                        } label: {
                            Label("Disconnect", systemImage: "link.slash")
                        }
                        .buttonStyle(.borderedProminent)
                    } else {
                        Button {
                            core.connect(to: core.targetIP, port: UInt16(core.targetPort) ?? 8800)
                        } label: {
                            Label("Connect", systemImage: "link")
                        }
                        .buttonStyle(.borderedProminent)
                        .disabled(core.targetIP.isEmpty)
                    }
                }
            }
            .padding()
            .background(Color(nsColor: .controlBackgroundColor).opacity(0.5))

            Divider()

            // Status bar
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

            Divider()

            // Content
            if core.isConnected {
                VStack {
                    if core.totalBytes > 0 || core.isBusy {
                        TransferProgressBar(status: core.transferStatus, progress: core.progress,
                                            transferred: core.transferredBytes, total: core.totalBytes)
                    }

                    // Drop zone
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

                    // Active transfers
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
            } else {
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
        }
    }

    private func handleDrop(_ providers: [NSItemProvider]) {
        for provider in providers {
            _ = provider.loadObject(ofClass: URL.self) { url, _ in
                guard let url else { return }
                DispatchQueue.main.async {
                    core.sendLocalFile(url.path)
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
                core.sendLocalFile(url.path)
            }
        }
    }
}

// MARK: - Transfer Log View

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
                                        Text("\(formatBytes(file.fileSize)) • \(file.fromIP) • \(timeFormatter.string(from: file.receivedAt))")
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

    private func logColor(_ msg: String) -> Color {
        if msg.contains("ERROR") { return .red }
        if msg.contains("complete") || msg.contains("connected") || msg.contains("started") { return .green }
        if msg.contains("fail") || msg.contains("cancel") { return .orange }
        return .secondary
    }
}
