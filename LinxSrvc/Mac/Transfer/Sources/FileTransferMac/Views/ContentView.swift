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
        if let fieldWidth = width {
            field.widthAnchor.constraint(equalToConstant: fieldWidth).isActive = true
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
            Picker("", selection: $selectedTab) {
                Label("Receive", systemImage: "arrow.down.circle").tag(0)
                Label("Send", systemImage: "arrow.up.circle").tag(1)
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
                LogConsole(messages: core.logMessages) {
                    core.logMessages.removeAll()
                }
                    .frame(height: 140)
            }
        }
    }
}
