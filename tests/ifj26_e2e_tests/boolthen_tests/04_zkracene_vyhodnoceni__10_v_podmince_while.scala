import ifj26.*

@main
def main(): Unit =
  var i = 0
  while i < 3 && 10 / (3 - i) > 0 do
    print(i)
    i = i + 1
