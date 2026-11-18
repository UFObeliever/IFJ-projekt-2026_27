import ifj26.*

@main def main(): Unit =
  var i = 0
  while i < 4 do
    if i / 2 * 2 == i then print("s") else print("l")
    i = i + 1
