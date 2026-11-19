import ifj26.*

@main
def main(): Unit =
  var count = 0
  for i <- 1 to 5 if count < 2 do
    count = count + 1
    print(i)
