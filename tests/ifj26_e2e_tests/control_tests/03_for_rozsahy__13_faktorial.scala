import ifj26.*

@main
def main(): Unit =
  var f = 1
  for i <- 1 to 5 do
    f = f * i
  print(f)
