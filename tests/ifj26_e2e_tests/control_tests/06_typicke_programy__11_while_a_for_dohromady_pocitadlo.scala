import ifj26.*

@main
def main(): Unit =
  var rounds = 0
  var total = 0
  while rounds < 3 do
    for i <- 1 to rounds + 1 do total = total + i
    rounds = rounds + 1
  print(total)
