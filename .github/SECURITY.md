# Security Policy

## Intended use

fastbrute is a security-research and password-auditing tool. It recovers the
5-character secret in the hashcat mode-4010 construction
`md5(challenge + hex(md5(challenge + secret)))`. It is intended for use only
against systems, hashes, and data you own or are explicitly authorized to test
(CTF challenges, your own credentials, sanctioned penetration tests, and
academic or defensive research). Using it against hashes or systems you do not
own or lack permission to test may be illegal. You are responsible for how you
use it.

## Scope

fastbrute is an offline, local tool. It makes no network requests, opens no
listening sockets, and reads no files beyond what you pass on the command line
or the Python API. The security-relevant surfaces are:

- The C extension boundary (untrusted argument lengths, thread creation, SIMD
  buffer bounds).
- The Odin CLI argument parsing (hex decoding into fixed buffers).
- Input validation on the public Python helpers.

## Supported versions

Security fixes target the latest release and the current `main` branch.

| Version | Supported |
|---------|-----------|
| 1.0.x   | Yes       |
| < 1.0   | No        |

## Reporting a vulnerability

Please do not open a public issue for a security vulnerability.

Report it privately through GitHub Security Advisories: open the repository's
**Security** tab and choose **Report a vulnerability**
(<https://github.com/TajuC/fastbrute/security/advisories/new>).

Include the affected version or commit, a description of the issue, and a minimal
reproduction if you have one. You can expect an initial response within a few
days.

## Hardening notes

- The native extension performs runtime CPUID dispatch with a scalar fallback;
  the ISA-specific `group8`/`group16` helpers validate their arguments and refuse
  to run on unsupported hardware rather than executing an illegal instruction.
- The Odin build ships with bounds checks disabled for throughput, so its CLI
  validates argument lengths and hex before decoding into fixed-size buffers.
- Releases publish a CycloneDX SBOM and `SHA256SUMS`; verify downloads against
  the checksums.
