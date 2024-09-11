# Obsidian-CLI

A CLI tool for managing Obsidian notes via neovim. WIP.

## Installing
Clone the directory, then compile using 
`gcc -o obs main.c utils.c`
Then move the resulting executable into `/usr/local/bin` or similar on MacOS, or any other directory either on system PATH, or add your own. Give execute permissions on the executable with
`sudo chmod +x /usr/local/bin/obs`

## Usage
Tool has three options currently (more to be added):
 - list
 - add
 - edit
`obs list` will list any notes associated with the current working directory. If the current working directory is a git repository then this will be under a path like `/<git organisation/user>/<repository name>`. Otherwise it will be under `/temp`.

`obs add` will create a new note in either `/temp` or the git path. The filename will be the current timestamp.

`obs edit <filename>` is used to edit a previously existing note in the current working directory. 

## Features in progress
 - Auto-complete file paths (tabbing) in the `obs edit <filename>`
 - Add obsidian links between notes in the same repo
 - Add a backup option to push repositories notes to git, use the correct git profile for work vs personal repositories 
 - A TUI similar to lazygit

