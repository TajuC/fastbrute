# Security Policy

## What fastbrute is for

fastbrute is a tool for security research and password auditing. It recovers the 5 character secret in the hashcat mode 4010 hash, `md5(challenge + hex(md5(challenge + secret)))`. Only use it against systems, hashes, and data that you own or are clearly allowed to test. That includes CTF challenges, your own credentials, penetration tests you were hired to run, and academic or defensive research. Using it against hashes or systems you do not own or lack permission to test may be illegal, and the responsibility for that sits with you.

## Scope

fastbrute runs offline and stays local. It makes no network requests, opens no listening sockets, and reads nothing beyond what you pass on the command line or through the Python API. The parts worth thinking about for security are the C extension boundary, where untrusted argument lengths, thread creation, and SIMD buffer bounds all live, the Odin command line parser, which decodes hex into fixed buffers, and the input validation on the public Python helpers.

## Supported versions

Security fixes target the latest release and the current main branch.

| Version | Supported |
|---------|-----------|
| 1.0.x   | Yes       |
| older   | No        |

## Reporting a vulnerability

Please do not open a public issue for a security problem.

Report it privately through GitHub Security Advisories. Open the Security tab on the repository and choose Report a vulnerability. The direct link is https://github.com/TajuC/fastbrute/security/advisories/new.

Tell us the affected version or commit, describe the issue, and include a small reproduction if you have one. You can expect a first reply within a few days.

## Hardening notes

The native extension detects the CPU at run time and always keeps a scalar fallback. The ISA specific group8 and group16 helpers check their arguments and refuse to run on hardware that cannot support them, rather than executing an instruction the CPU does not have. The Odin build ships with bounds checks turned off for speed, so its command line validates argument lengths and hex before it decodes anything into a fixed buffer. Every release publishes a CycloneDX SBOM and a SHA256SUMS file, so verify your download against the checksums.
