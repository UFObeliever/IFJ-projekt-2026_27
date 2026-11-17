import ifj26.*

@main
def main(): Unit =
  var running = true
  var i = 0
  while running do
    i = i + 1
    if i == 3 then
      running = false
  print(i)
