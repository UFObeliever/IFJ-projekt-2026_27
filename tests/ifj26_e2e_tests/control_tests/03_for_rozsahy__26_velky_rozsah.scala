import ifj26.*

@main
def main(): Unit =
  var total = 0
  for i <- 1 to 1000 do
    total = total + i
  print(total)
