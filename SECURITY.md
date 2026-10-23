# Security Policy

## Supported versions

Only the latest release on `main` receives security fixes.

## Reporting a vulnerability

Please **do not** open a public issue for security problems.

Email **[INSERT SECURITY CONTACT]** with:

- a description of the issue and its impact,
- steps or a file that reproduces it (for example a crafted catalogue CSV),
- the version or commit you tested.

You should get an acknowledgement within 72 hours. We'll keep you updated on
the fix and credit you in the release notes unless you prefer otherwise.

## Scope

myna parses untrusted catalogue files, so memory-safety bugs in
`src/core/catalogue.c` (overflows, out-of-bounds reads) are in scope.
