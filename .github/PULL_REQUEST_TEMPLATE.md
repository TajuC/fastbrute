## Summary

Explain what this change does and why.

## Changes

List the notable changes.

## Testing

- [ ] `python tests/test_correctness.py` passes for both native and pure Python
- [ ] `ruff check .` is clean
- [ ] Odin `./sf test` passes if you touched odin/
- [ ] SIMD kernel changes keep the hashlib reference tests green on every ISA tier

## Performance

If your change affects speed, note the machine, the ISA tier you set through FASTBRUTE_ISA, the before and after numbers from bench.py, and any thermal caveats.

## Notes

Anything else reviewers should know, such as tradeoffs or follow ups.
