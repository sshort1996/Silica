# Obsidian-CLI

A CLI tool for managing Obsidian notes via neovim. WIP.

## Installation
### Building locally
We'll look into making this available through brew eventually, but there's some work left to do until that point. For now the compiled executables are available in the `build` directory, and source code in `src`.

Clone the directory, then compile using 
`gcc -Iutils -o build/main src/main.c utils/utils.c -lreadline`
Then move the resulting executable into `/usr/local/bin` or similar on MacOS, or any other directory either on system PATH, or add your own. Give execute permissions on the executable with
`sudo chmod +x /usr/local/bin/obs`

## Usage
Tool has three options currently (more to be added):
 - list
 - add
 - edit
`obs list` will list any notes associated with the current working directory. If the current working directory is a git repository then this will be under a path like `/<git organisation/user>/<repository name>`. Otherwise it will be under `/temp`.

`obs add` will create a new note in either `/temp` or the git path. The filename will be the current timestamp.

`obs edit` is used to edit a previously existing note in the current working directory. This option uses the `readline` tool to give auto-complete suggestions for the file paths in the target directory.

## Features in progress
 - Add obsidian links between notes in the same repo
 - Add a backup option to push repositories notes to git, use the correct git profile for work vs personal repositories 
 - A TUI similar to lazygit
 - Integrate OpenAI API queries to name files based on the file contents rather than by timestamp.
 - Add summary files to each directory which summarise each file, and creates obsidian links to the respective notes.
