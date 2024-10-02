# Obsidian-CLI

A CLI tool for managing Obsidian notes via neovim. WIP.

## Installing
Clone the directory, then compile using 
`gcc -Iutils -o build/main src/main.c utils/utils.c -lreadline`
Then move the resulting executable into `/usr/local/bin` or similar on MacOS, or any other directory either on system PATH, or add your own. Give execute permissions on the executable with
`sudo chmod +x /usr/local/bin/obs`

## Usage
Tool has four options currently (more to be added):
 - config
 - list
 - add
 - edit
`obs config` is used to initially set the location of your obsidian vault, and your OpenAI API key (to be used for parsing file contents and renaming files).
`obs list` will list any notes associated with the current working directory. If the current working directory is a git repository then this will be under a path like `/<git organisation/user>/<repository name>`. Otherwise it will be under `/temp`.

`obs add` will create a new note in either `/temp` or the git path. The filename will be the current timestamp.

`obs edit <filename>` is used to edit a previously existing note in the current working directory. This option uses the `readline` tool to give auto-complete suggestions for the file paths in the target directory.

## Features in progress
 - Add obsidian links between notes in the same repo
 - Add a backup option to push repositories notes to git, use the correct git profile for work vs personal repositories 
 - A TUI similar to lazygit

