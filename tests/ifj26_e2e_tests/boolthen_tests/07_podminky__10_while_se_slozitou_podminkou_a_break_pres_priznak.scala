import ifj26.*

@main
def main(): Unit =
  var i = 1
  var found = false
  while i <= 10 && !found do
    if i * i > 50 then
      found = true
    else
      i = i + 1
  print(i)
