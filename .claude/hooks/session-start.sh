#!/bin/bash
# Install the Claude Code plugins that .claude/settings.json enables, so their
# skills are available in Claude Code cloud sessions. Local machines install
# them through the normal /plugin prompt, so this only runs remotely.
set -euo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

cd "${CLAUDE_PROJECT_DIR:-.}"

plugins_dir="${HOME}/.claude/plugins"
marketplace="mattpocock"
marketplace_source="mattpocock/skills"
plugin="mattpocock-skills@${marketplace}"

# Skip when the plugin is already installed (the container state is cached).
if grep -q "\"${plugin}\"" "${plugins_dir}/installed_plugins.json" 2>/dev/null; then
  exit 0
fi

# User scope on purpose: project scope rewrites .claude/settings.json and would
# leave the working tree dirty every session. The project settings already
# declare the marketplace and enable the plugin.
if ! grep -q "\"${marketplace}\"" "${plugins_dir}/known_marketplaces.json" 2>/dev/null; then
  claude plugin marketplace add "${marketplace_source}" --scope user >&2
fi
claude plugin install "${plugin}" --scope user >&2
