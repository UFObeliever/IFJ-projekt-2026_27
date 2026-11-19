import ifj26.*

@main
def main(): Unit =
  var found = 0
  for i <- 10 to 30 if i / 3 * 3 == i if found == 0 do found = i
  print(found)
