Code Similarity Analysis Tool

This tool analyzes code similarity between files or directories, ignoring comments and whitespace. It provides detailed reports on matching and unmatched files with original/effective line counts.

Key Features:
- Compares code files or entire directories
- Ignores comments and whitespace for accurate similarity calculation
- Shows color-coded differences with line numbers
- Supports ignoring specific directories and file patterns
- Displays both original and effective (non-comment) line counts
- Reports unmatched files with their line statistics

Installation:
1. Ensure Python 3.6+ is installed
2. Install required packages: pip install colorama difflib

Usage:
Basic file comparison:
python similarity.py source_file target_file [--diff] [--no-color]

Directory comparison:
python similarity.py source_dir target_dir [--diff] [--no-color] [--ignore-dirs dir1 dir2] [--ignore-files pattern1 pattern2]

Arguments:
source          Source file/directory path
target          Target file/directory path
--diff          Show detailed differences
--no-color      Disable colored output
--ignore-dirs   Directories to ignore (space-separated)
--ignore-files  File patterns to ignore (space-separated)

Examples:
1. Compare two files with colored diff:
python similarity.py file1.js file2.js --diff

2. Compare directories ignoring common temp files:
python similarity.py projectA projectB --ignore-dirs venv __pycache__ --ignore-files *.tmp *.bak

3. Compare without color output:
python similarity.py src lib --no-color

Output Sections:
1. Unmatched Files (source only)
   - Lists files only in source directory
   - Shows original and effective line counts

2. Unmatched Files (target only)
   - Lists files only in target directory
   - Shows original and effective line counts

3. Matched Files
   - Shows similarity percentage for each file pair
   - Original and effective line counts for both versions
   - Color-coded by similarity (green >70%, yellow 30-70%, red <30%)

4. Summary Analysis
   - Total matched files count
   - Total unmatched files count
   - Overall similarity percentage
   - Total line statistics (original and effective)
   - Total duplicate lines count

Note: Effective lines = Original lines - Comments - Blank lines
Similarity is calculated using effective lines only for accuracy
