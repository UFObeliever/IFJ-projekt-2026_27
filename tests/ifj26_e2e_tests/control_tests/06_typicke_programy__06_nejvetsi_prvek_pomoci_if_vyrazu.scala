import ifj26.*

@main
def main(): Unit =
  var best = 0
  for i <- 1 to 10 do
    val v = i * (11 - i)
    best = if v > best then v else best
  print(best)
