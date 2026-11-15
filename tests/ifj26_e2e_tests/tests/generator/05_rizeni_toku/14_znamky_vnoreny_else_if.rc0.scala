import ifj26.*

@main def main(): Unit =
  var score = 0
  var k = 0
  while k < 4 do
    if k == 0 then
      score = 95
    else
      if k == 1 then
        score = 85
      else
        if k == 2 then
          score = 75
        else
          score = 50
    if score >= 90 then
      print("A")
    else
      if score >= 80 then
        print("B")
      else
        if score >= 70 then
          print("C")
        else
          print("F")
    k = k + 1
  print("\n")
