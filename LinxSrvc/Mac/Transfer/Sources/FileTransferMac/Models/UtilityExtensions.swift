import Foundation

// MARK: - Byte formatting

func formatBytes(_ bytes: UInt64) -> String {
    if bytes >= 1_073_741_824 {
        String(format: "%.1f GB", Double(bytes) / 1_073_741_824.0)
    } else if bytes >= 1_048_576 {
        String(format: "%.1f MB", Double(bytes) / 1_048_576.0)
    } else if bytes >= 1_024 {
        String(format: "%.1f KB", Double(bytes) / 1_024.0)
    } else {
        "\(bytes) B"
    }
}

// MARK: - Date formatter

let timeFormatter: DateFormatter = {
    let f = DateFormatter()
    f.dateStyle = .none
    f.timeStyle = .short
    return f
}()
