import ifj26.*

@main
def main(): Unit =
  var i = 0
  var evens = 0
  while i < 10 do
    if i / 2 * 2 == i then
      evens = evens + 1
    i = i + 1
  print(evens)
