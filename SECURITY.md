# Security Policy

This is a hobby/academic hardware project with no formal support. To report a vulnerability, use GitHub's private vulnerability reporting for this repository if it is enabled by the owner; otherwise open an issue **without** exploit details or secrets and ask for a private channel.

Never post Wi-Fi passwords, API keys or tokens in issues or PRs.

Known limitations: weather requests use plain HTTP by default (API key unencrypted); HTTPS needs a user-supplied root CA; credentials are compiled into the firmware image (anyone with the board's flash can read them); a key was exposed in earlier Git history and must be revoked; the firmware has no authentication or OTA features; the pump control is not a substitute for hardware safety measures.
