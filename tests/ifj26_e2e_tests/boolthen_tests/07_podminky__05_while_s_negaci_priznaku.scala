import ifj26.*

@main
def main(): Unit =
  var done = false
  var i = 0
  while !done do
    i = i + 1
    if i >= 4 then
      done = true
  print(i)
