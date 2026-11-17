import ifj26.*

@main
def main(): Unit =
  var s = 1
  for i <- 1 to 5 by s do
    s = s + 1
    print(i)
