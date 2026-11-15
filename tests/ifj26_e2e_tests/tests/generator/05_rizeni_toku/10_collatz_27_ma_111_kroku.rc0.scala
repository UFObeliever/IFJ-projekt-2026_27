import ifj26.*

@main def main(): Unit =
  var n = 27
  var steps = 0
  var peak = 27
  while n != 1 do
    val r = n - n / 2 * 2
    if r == 0 then
      n = n / 2
    else
      n = 3 * n + 1
    if n > peak then
      peak = n
    else
      peak = peak
    steps = steps + 1
  print(steps)
  print("\n")
  print(peak)
  print("\n")
