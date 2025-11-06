#!/usr/bin/env bash
set -euo pipefail

TASKS_JSON="$(dirname "$0")/tasks.json"

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <task_name>" >&2
  exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
  echo "jq is required to run tasks. Please install jq." >&2
  exit 1
fi

declare -A TASK_MARK

run_task() {
  local task_name="$1"

  if [[ -n "${TASK_MARK["done:${task_name}"]:-}" ]]; then
    return
  fi

  if [[ -n "${TASK_MARK["visiting:${task_name}"]:-}" ]]; then
    echo "Dependency cycle detected at task '${task_name}'." >&2
    exit 1
  fi

  local exists
  exists=$(jq -r --arg name "${task_name}" \
    '.tasks[] | select(.name == $name) | "yes"' "${TASKS_JSON}" || true)

  if [[ "${exists}" != "yes" ]]; then
    echo "Task '${task_name}' not found." >&2
    exit 1
  fi

  TASK_MARK["visiting:${task_name}"]=1

  mapfile -t deps < <(jq -r --arg name "${task_name}" \
    '.tasks[] | select(.name == $name) | (.depends_on // [])[]' "${TASKS_JSON}" || true)

  for dep in "${deps[@]}"; do
    [[ -z "${dep}" ]] && continue
    run_task "${dep}"
  done

  local command
  command=$(jq -r --arg name "${task_name}" \
    '.tasks[] | select(.name == $name) | .command // empty' "${TASKS_JSON}")

  if [[ -z "${command}" ]]; then
    echo "Task '${task_name}' has no command defined." >&2
    exit 1
  fi

  echo ">> ${task_name}"
  eval "${command}"

  TASK_MARK["done:${task_name}"]=1
  unset TASK_MARK["visiting:${task_name}"]
}

run_task "$1"
