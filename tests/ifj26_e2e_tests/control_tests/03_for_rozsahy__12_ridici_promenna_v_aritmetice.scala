import ifj26.*

@main
def main(): Unit =
  var sum = 0
  for i <- 1 to 4 do
    sum = sum + i * i
  print(sum)
