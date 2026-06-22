# Documentation Development & Local Preview

This directory contains the source documentation for the project, which is automatically built and deployed via GitHub Actions.

> [!NOTE]
> This site uses GitHub Pages configuration layouts. When compiling your changes locally, you must serve the directory using the matching repository `--baseurl` argument. Otherwise, your CSS styles, asset links, and cross-file references will fail to load properly.

Follow these steps to preview changes locally in WSL (Windows Subsystem for Linux) using the exact configuration matching the live GitHub Pages environment before pushing your updates.

---

## Prerequisites (One-time Setup)

Ensure Ruby and its compilation tools are installed on your WSL distro:
```bash
sudo apt update
sudo apt install ruby-full build-essential zlib1g-dev -y
```

---

## Local Preview Steps

Execute these commands from the **root directory** of the repository:

### 1. Set Up the Local Environment

Initialize a clean local bundle configuration (this isolates the workspace and installs dependencies locally without altering your system gems):

```bash
# Set up a local tracking folder
bundle config set --local path 'vendor/bundle'

# Install matching GitHub pipeline dependencies
bundle install
```

*(Note: Ensure `vendor/` is added to your root `.gitignore` file so these local framework files are not committed).*

### 2. Launch the Local Jekyll Server

Run the preview server matching the target production subfolder context:

```bash
bundle exec jekyll serve --source docs --baseurl /imx93-edge-lab
```

### 3. View the Documentation

Open your browser and navigate to:
[http://localhost:4000/imx93-edge-lab/](https://www.google.com/search?q=http://localhost:4000/imx93-edge-lab/)

* **Live Reloading:** Any modifications made to the `.md` files in this directory will be automatically compiled. Simply refresh the browser page to view your edits.
* **Stop Server:** Press `Ctrl + C` in the terminal to close the preview server.