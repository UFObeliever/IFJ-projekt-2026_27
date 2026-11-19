import ifj26.*

@main
def main(): Unit =
  for i <- 1 to 2 do
    var j = 0
    while j < i do
      print(j)
      j = j + 1
    print("|")
