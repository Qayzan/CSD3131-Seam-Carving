# Shared interface contract (v1 — implemented in Milestone 2)

This is the agreement that lets FF, HM and WN build their seam finders independently and still plug them into the same carver, tests, benchmarks and demo. Sections 1–2 are implemented in `src/seam_carving/` (Milestone 2) and section 3 (`carve`) in Milestone 3. **HM and WN: please still review §5 and comment on the Milestone 2 pull request.** Changes after that need a pull request reviewed by FF.

## 1. Arrays

| Thing | Type | Shape | Notes |
|---|---|---|---|
| Colour image | `np.ndarray`, `uint8` | `(H, W, 3)` | **RGB** order (converted from OpenCV's BGR by `image_utils.load_image`) |
| Greyscale image | `np.ndarray`, `uint8` | `(H, W)` | accepted everywhere a colour image is |
| Energy map | `np.ndarray`, `float64` | `(H, W)` | non-negative; produced by `energy.gradient_energy` |

Indexing is always `array[row, col]`. Row 0 is the top row and column 0 is the left column.

## 2. Types (implemented in `seam_carving/interfaces.py`)

```python
class Orientation(enum.Enum):
    VERTICAL = "vertical"  # top → bottom; removes one pixel from every ROW (width − 1)
    HORIZONTAL = "horizontal"  # left → right; removes one pixel from every COLUMN (height − 1)


@dataclass(frozen=True)
class Seam:
    orientation: Orientation
    indices: np.ndarray  # int64
    # VERTICAL:   len == H, indices[i] = column removed in row i
    # HORIZONTAL: len == W, indices[j] = row removed in column j
    energy: float  # sum of energy along the seam (same energy map the finder received)


class SeamFinder(Protocol):
    def __call__(self, energy: np.ndarray, orientation: Orientation) -> Seam: ...
```

Rules every `SeamFinder` must follow:

1. It must **not modify** `energy` (the shared carver reuses it for visualisation).
2. The returned seam must be **valid**: correct length; indices in range; consecutive indices differ by at most 1. `seam_ops.validate_seam` checks this, and the carver calls it after every finder call.
3. `Seam.energy` must equal `seam_ops.seam_energy(energy, seam)` (within floating-point tolerance).
4. Results must be deterministic for the same input.
5. It must not use any library content-aware resizing / seam-carving function (brief rule).

Each algorithm module exposes one public entry point with this signature:

```python
# dynamic_programming.py (FF), greedy.py (HM), graph_cut.py (WN)
def find_seam(energy: np.ndarray, orientation: Orientation) -> Seam: ...
```

Algorithm-specific extras (the DP table M, a residual graph, …) go in optional helper functions or a `debug=True` variant. They are not part of the shared signature.

What the implementation adds to the sketch above:

- `Seam(...)` copies `indices` into a new **read-only `int64`** array, so any integer dtype is accepted (for example the `intp` that `np.argmin` returns). Floats, booleans, empty or 2-D arrays raise `TypeError`/`ValueError` straight away. `energy` is required and stored as a Python `float`.
- Two seams are equal when orientation, indices and energy are all equal (seams are deliberately unhashable).
- `len(seam)` is the number of pixels, and `seam.coordinates()` returns `(rows, cols)` for direct indexing: `energy[seam.coordinates()]`.

### Helpers every finder can use (`seam_carving.seam_ops`)

| Function | What it does |
|---|---|
| `validate_seam(seam, shape, *, energy=None)` | Raises `ValueError` with a precise message unless the seam fits `shape` (length, range, 8-connectivity). With `energy=` it also checks contract rule 3 (`seam.energy` equals the sum along the seam). Returns `None` when valid. |
| `seam_energy(energy, seam)` | Sum of the energy map along the seam (Python `float`). |
| `seam_mask(shape, seam)` | Boolean `(H, W)` mask, `True` exactly on the seam. |
| `remove_seam(image, seam)` | New array one column (vertical) or row (horizontal) smaller; dtype, channels and the order of all other pixels kept. |
| `highlight_seam(image, seam, colour=(255, 0, 0))` | RGB `uint8` copy with the seam painted, for figures and the demo. |

Energy and images: `energy.gradient_energy(image)` gives the `float64 (H, W)` map; `image_utils.load_image` / `save_image` handle files (RGB inside the package, BGR only on disk).

**Quick self-check for HM / WN** (put it in your tests):

```python
from seam_carving.energy import gradient_energy
from seam_carving.interfaces import Orientation
from seam_carving.seam_ops import validate_seam

energy = gradient_energy(image)
for orientation in Orientation:
    seam = find_seam(energy, orientation)
    validate_seam(seam, energy.shape, energy=energy)  # raises if anything is off
```

## 3. Results of a full resize

```python
@dataclass
class CarveResult:
    image: np.ndarray  # resized image, same dtype and channels as the input
    seams: list[Seam]  # in removal order, each in the coordinates of the image at that step
    timings: dict[str, float]  # seconds: "energy", "find_seam", "remove", "total"
    frames: list[np.ndarray] | None  # optional intermediate images (visualisation / bonus 7a)
```

`carver.carve(image, target_height, target_width, finder=None, order="width_first", keep_frames=False, *, energy_fn=gradient_energy) -> CarveResult` (implemented in Milestone 3)

- Raises `ValueError` if a target is larger than the current size, or less than 1 (`TypeError` if it is not an integer).
- Recomputes the energy map before **every** seam (brief R1c).
- `finder=None` means the DP finder (Q1). Pass `greedy.find_seam` or `graph_cut.find_seam` to compare.
- `order`: `"width_first"`, `"height_first"` or `"alternate"`.
- After every finder call the carver checks the contract: the energy map is handed over **read-only** (writing to it raises `ValueError`, rule 1), and `validate_seam(seam, E.shape, energy=E)` checks validity and `Seam.energy` (rules 2–3). A finder that breaks a rule fails immediately with a clear message.
- `keep_frames=True`: `frames[0]` is the input and `frames[k]` the image after seam k (`len(frames) == len(seams) + 1`).
- `energy_fn` lets tests count energy recomputations, and lets the report try other energy functions.
- `carver.seams_in_original_coordinates(original_shape, result.seams)` maps every removed seam back to pixel positions in the original picture (used to draw all seams at once, bonus 7a).

## 4. Timing

Timings use `time.perf_counter()` around the finder call only, measured by the carver. This means the DP, greedy and graph cut are timed the same way, which matters for the Q3/Q4/Q5 comparisons.

## 5. Questions for reviewers

- HM / WN: is `find_seam(energy, orientation)` enough for your algorithm? Graph cut may prefer the image for forward energy. If so, we add an optional `image=` keyword rather than changing the shared signature.
- ~~Should `Seam.indices` be `int64` or `intp`?~~ Implemented as `int64` (any integer input is converted), for predictable serialisation. Shout if this causes trouble.
