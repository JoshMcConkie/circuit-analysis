# circuit-analysis

A small C++20 circuit solver experiment that builds simple circuits and solves node voltages using an MNA-style linear system.

Supported components:
- Resistors
- Independent current sources
- Independent voltage sources (via constraint rows/cols)

---

## Files

- `components.h` — circuit + component definitions and solver
- `circuit_test.cpp` — example usage / quick test

---

## Requirements

- C++20 compiler
- Eigen (linear algebra)

### Arch Linux

```bash
sudo pacman -S eigen
```

Eigen headers are typically in `/usr/include/eigen3`.

---

## Build & run

```bash
g++ -std=c++20 -O2 -I/usr/include/eigen3 circuit_test.cpp -o circuit_test
./circuit_test
```

You should see a voltage vector printed and then:

```
All good.
```

---

## Notes

Voltage source sign convention:

`VoltageSource(a, b, V)` enforces:

**V(b) − V(a) = V**

(i.e., `a` is the negative terminal, `b` is the positive terminal).

---

## License

MIT Standard License.
